#!/usr/bin/env python3
"""遞迴下降 6502 反組譯器(分離 code/data,給 Ultima I AI 逆向用)。
從進入點跟控制流(JSR/JMP/分支/fall-through)走,只把真正到達的 byte 當 code,
其餘留 data。輸出反組譯 + 子程式呼叫圖 + ZP 存取統計。

用法:dis6502.py <flat.bin> <base_hex> [entry_hex ...]
flat.bin 是已攤平的記憶體映像(見 atari_extract 重建);base 是它的起始位址。"""
import sys

# opcode -> (mnemonic, mode)  mode 決定運算元長度與是否控制流
# 模式:imp/acc=0, imm/zp/zpx/zpy/indx/indy/rel=1, abs/absx/absy/ind=2
M = {  # 精簡但完整覆蓋合法 6502
 0x00:("brk","imp"),0x01:("ora","indx"),0x05:("ora","zp"),0x06:("asl","zp"),0x08:("php","imp"),
 0x09:("ora","imm"),0x0A:("asl","acc"),0x0D:("ora","abs"),0x0E:("asl","abs"),
 0x10:("bpl","rel"),0x11:("ora","indy"),0x15:("ora","zpx"),0x16:("asl","zpx"),0x18:("clc","imp"),
 0x19:("ora","absy"),0x1D:("ora","absx"),0x1E:("asl","absx"),
 0x20:("jsr","abs"),0x21:("and","indx"),0x24:("bit","zp"),0x25:("and","zp"),0x26:("rol","zp"),
 0x28:("plp","imp"),0x29:("and","imm"),0x2A:("rol","acc"),0x2C:("bit","abs"),0x2D:("and","abs"),0x2E:("rol","abs"),
 0x30:("bmi","rel"),0x31:("and","indy"),0x35:("and","zpx"),0x36:("rol","zpx"),0x38:("sec","imp"),
 0x39:("and","absy"),0x3D:("and","absx"),0x3E:("rol","absx"),
 0x40:("rti","imp"),0x41:("eor","indx"),0x45:("eor","zp"),0x46:("lsr","zp"),0x48:("pha","imp"),
 0x49:("eor","imm"),0x4A:("lsr","acc"),0x4C:("jmp","abs"),0x4D:("eor","abs"),0x4E:("lsr","abs"),
 0x50:("bvc","rel"),0x51:("eor","indy"),0x55:("eor","zpx"),0x56:("lsr","zpx"),0x58:("cli","imp"),
 0x59:("eor","absy"),0x5D:("eor","absx"),0x5E:("lsr","absx"),
 0x60:("rts","imp"),0x61:("adc","indx"),0x65:("adc","zp"),0x66:("ror","zp"),0x68:("pla","imp"),
 0x69:("adc","imm"),0x6A:("ror","acc"),0x6C:("jmp","ind"),0x6D:("adc","abs"),0x6E:("ror","abs"),
 0x70:("bvs","rel"),0x71:("adc","indy"),0x75:("adc","zpx"),0x76:("ror","zpx"),0x78:("sei","imp"),
 0x79:("adc","absy"),0x7D:("adc","absx"),0x7E:("ror","absx"),
 0x81:("sta","indx"),0x84:("sty","zp"),0x85:("sta","zp"),0x86:("stx","zp"),0x88:("dey","imp"),
 0x8A:("txa","imp"),0x8C:("sty","abs"),0x8D:("sta","abs"),0x8E:("stx","abs"),
 0x90:("bcc","rel"),0x91:("sta","indy"),0x94:("sty","zpx"),0x95:("sta","zpx"),0x96:("stx","zpy"),
 0x98:("tya","imp"),0x99:("sta","absy"),0x9A:("txs","imp"),0x9D:("sta","absx"),
 0xA0:("ldy","imm"),0xA1:("lda","indx"),0xA2:("ldx","imm"),0xA4:("ldy","zp"),0xA5:("lda","zp"),
 0xA6:("ldx","zp"),0xA8:("tay","imp"),0xA9:("lda","imm"),0xAA:("tax","imp"),0xAC:("ldy","abs"),
 0xAD:("lda","abs"),0xAE:("ldx","abs"),
 0xB0:("bcs","rel"),0xB1:("lda","indy"),0xB4:("ldy","zpx"),0xB5:("lda","zpx"),0xB6:("ldx","zpy"),
 0xB8:("clv","imp"),0xB9:("lda","absy"),0xBA:("tsx","imp"),0xBC:("ldy","absx"),0xBD:("lda","absx"),0xBE:("ldx","absy"),
 0xC0:("cpy","imm"),0xC1:("cmp","indx"),0xC4:("cpy","zp"),0xC5:("cmp","zp"),0xC6:("dec","zp"),
 0xC8:("iny","imp"),0xC9:("cmp","imm"),0xCA:("dex","imp"),0xCC:("cpy","abs"),0xCD:("cmp","abs"),0xCE:("dec","abs"),
 0xD0:("bne","rel"),0xD1:("cmp","indy"),0xD5:("cmp","zpx"),0xD6:("dec","zpx"),0xD8:("cld","imp"),
 0xD9:("cmp","absy"),0xDD:("cmp","absx"),0xDE:("dec","absx"),
 0xE0:("cpx","imm"),0xE1:("sbc","indx"),0xE4:("cpx","zp"),0xE5:("sbc","zp"),0xE6:("inc","zp"),
 0xE8:("inx","imp"),0xE9:("sbc","imm"),0xEA:("nop","imp"),0xEC:("cpx","abs"),0xED:("sbc","abs"),0xEE:("inc","abs"),
 0xF0:("beq","rel"),0xF1:("sbc","indy"),0xF5:("sbc","zpx"),0xF6:("inc","zpx"),0xF8:("sed","imp"),
 0xF9:("sbc","absy"),0xFD:("sbc","absx"),0xFE:("inc","absx"),
}
LEN={"imp":1,"acc":1,"imm":2,"zp":2,"zpx":2,"zpy":2,"indx":2,"indy":2,"rel":2,"abs":3,"absx":3,"absy":3,"ind":3}

def main():
    binf, base = sys.argv[1], int(sys.argv[2],16)
    entries = [int(x,16) for x in sys.argv[3:]] or [base]
    data = open(binf,"rb").read()
    end = base + len(data)
    def rd(a): return data[a-base] if base<=a<end else None

    code=set(); insns={}; jsr_targets={}; stack=list(entries)
    while stack:
        a=stack.pop()
        while base<=a<end and a not in code:
            op=rd(a); ent=M.get(op)
            if ent is None: break          # 非法 opcode → 視為 data,停
            mn,mode=ent; ln=LEN[mode]
            if a+ln>end: break
            code.add(a)
            for k in range(1,ln): code.add(a+k)
            operand=None
            if ln==2: operand=rd(a+1)
            elif ln==3: operand=rd(a+1)|(rd(a+2)<<8)
            insns[a]=(mn,mode,operand)
            nxt=a+ln
            if mn in ("jmp",) and mode=="abs":
                if base<=operand<end: stack.append(operand)
                break
            if mn=="jmp" and mode=="ind": break
            if mn in ("rts","rti","brk"): break
            if mn=="jsr":
                jsr_targets[operand]=jsr_targets.get(operand,0)+1
                if base<=operand<end: stack.append(operand)
            if mode=="rel":
                tgt=a+2+((operand^0x80)-0x80)
                if base<=tgt<end: stack.append(tgt)
            a=nxt
    # 輸出
    print(f"; recursive disasm of {binf} base=${base:04X} entries={[hex(e) for e in entries]}")
    print(f"; reached {len(code)} code bytes / {len(data)} total ({100*len(code)//len(data)}%)")
    print(f"; subroutines (JSR targets, by call count):")
    for t,c in sorted(jsr_targets.items(), key=lambda kv:-kv[1])[:40]:
        inrange = "" if base<=t<end else " (extern)"
        print(f";   ${t:04X}  x{c}{inrange}")
    print()
    a=base
    while a<end:
        if a in insns:
            mn,mode,operand=insns[a]
            if mode in ("imp","acc"): txt=mn
            elif mode=="imm": txt=f"{mn} #${operand:02X}"
            elif mode in("zp","zpx","zpy","indx","indy"):
                sfx={"zpx":",x","zpy":",y","indx":",x)","indy":"),y"}.get(mode,"")
                pre="(" if mode in("indx","indy") else ""
                txt=f"{mn} {pre}${operand:02X}{sfx}"
            elif mode=="rel":
                tgt=a+2+((operand^0x80)-0x80); txt=f"{mn} ${tgt:04X}"
            else:
                sfx={"absx":",x","absy":",y","ind":")"}.get(mode,"")
                pre="(" if mode=="ind" else ""
                txt=f"{mn} {pre}${operand:04X}{sfx}"
            ln=LEN[mode]
            print(f"${a:04X}:  {txt}")
            a+=ln
        else:
            # data byte
            print(f"${a:04X}:  .byte ${rd(a):02X}")
            a+=1

if __name__=="__main__": main()
