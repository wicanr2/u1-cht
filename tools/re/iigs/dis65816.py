#!/usr/bin/env python3
"""dis65816.py — 65816 反組譯器(含 M/X 旗標寬度追蹤),供 ULTIMAI 逆向用。
方法論:借 6502-re-methodology.md;65816 多了 REP/SEP 改 A、X/Y 8/16 寬 → 追蹤旗標決定 imm 寬度。
用法:dis65816.py <bin> <start_hex> <count> [--mx MX]   # MX: 初始旗標 (0x30=A/XY 16-bit)
"""
import sys

# 定址模式 → 運算元 byte 數(M/X 依旗標)。'M'/'X' = imm 依旗標、'#'=固定1
M = {  # addressing mode : operand length (or 'M'/'X' for flag-dependent imm)
 'imp':0,'A':0,'imm_m':'M','imm_x':'X','imm8':1,'dp':1,'dpx':1,'dpy':1,
 'idp':1,'idx':1,'idy':1,'idl':1,'idly':1,'sr':1,'sry':1,
 'abs':2,'absx':2,'absy':2,'ind':2,'iax':2,'absl':3,'abslx':3,'ial':2,
 'rel8':1,'rel16':2,'bm':2,
}
# opcode → (mnemonic, mode)
OPC = {
0x00:('BRK','imm8'),0x01:('ORA','idx'),0x02:('COP','imm8'),0x03:('ORA','sr'),0x04:('TSB','dp'),0x05:('ORA','dp'),0x06:('ASL','dp'),0x07:('ORA','idl'),
0x08:('PHP','imp'),0x09:('ORA','imm_m'),0x0A:('ASL','A'),0x0B:('PHD','imp'),0x0C:('TSB','abs'),0x0D:('ORA','abs'),0x0E:('ASL','abs'),0x0F:('ORA','absl'),
0x10:('BPL','rel8'),0x11:('ORA','idy'),0x12:('ORA','idp'),0x13:('ORA','sry'),0x14:('TRB','dp'),0x15:('ORA','dpx'),0x16:('ASL','dpx'),0x17:('ORA','idly'),
0x18:('CLC','imp'),0x19:('ORA','absy'),0x1A:('INC','A'),0x1B:('TCS','imp'),0x1C:('TRB','abs'),0x1D:('ORA','absx'),0x1E:('ASL','absx'),0x1F:('ORA','abslx'),
0x20:('JSR','abs'),0x21:('AND','idx'),0x22:('JSL','absl'),0x23:('AND','sr'),0x24:('BIT','dp'),0x25:('AND','dp'),0x26:('ROL','dp'),0x27:('AND','idl'),
0x28:('PLP','imp'),0x29:('AND','imm_m'),0x2A:('ROL','A'),0x2B:('PLD','imp'),0x2C:('BIT','abs'),0x2D:('AND','abs'),0x2E:('ROL','abs'),0x2F:('AND','absl'),
0x30:('BMI','rel8'),0x31:('AND','idy'),0x32:('AND','idp'),0x33:('AND','sry'),0x34:('BIT','dpx'),0x35:('AND','dpx'),0x36:('ROL','dpx'),0x37:('AND','idly'),
0x38:('SEC','imp'),0x39:('AND','absy'),0x3A:('DEC','A'),0x3B:('TSC','imp'),0x3C:('BIT','absx'),0x3D:('AND','absx'),0x3E:('ROL','absx'),0x3F:('AND','abslx'),
0x40:('RTI','imp'),0x41:('EOR','idx'),0x42:('WDM','imm8'),0x43:('EOR','sr'),0x44:('MVP','bm'),0x45:('EOR','dp'),0x46:('LSR','dp'),0x47:('EOR','idl'),
0x48:('PHA','imp'),0x49:('EOR','imm_m'),0x4A:('LSR','A'),0x4B:('PHK','imp'),0x4C:('JMP','abs'),0x4D:('EOR','abs'),0x4E:('LSR','abs'),0x4F:('EOR','absl'),
0x50:('BVC','rel8'),0x51:('EOR','idy'),0x52:('EOR','idp'),0x53:('EOR','sry'),0x54:('MVN','bm'),0x55:('EOR','dpx'),0x56:('LSR','dpx'),0x57:('EOR','idly'),
0x58:('CLI','imp'),0x59:('EOR','absy'),0x5A:('PHY','imp'),0x5B:('TCD','imp'),0x5C:('JML','absl'),0x5D:('EOR','absx'),0x5E:('LSR','absx'),0x5F:('EOR','abslx'),
0x60:('RTS','imp'),0x61:('ADC','idx'),0x62:('PER','rel16'),0x63:('ADC','sr'),0x64:('STZ','dp'),0x65:('ADC','dp'),0x66:('ROR','dp'),0x67:('ADC','idl'),
0x68:('PLA','imp'),0x69:('ADC','imm_m'),0x6A:('ROR','A'),0x6B:('RTL','imp'),0x6C:('JMP','ind'),0x6D:('ADC','abs'),0x6E:('ROR','abs'),0x6F:('ADC','absl'),
0x70:('BVS','rel8'),0x71:('ADC','idy'),0x72:('ADC','idp'),0x73:('ADC','sry'),0x74:('STZ','dpx'),0x75:('ADC','dpx'),0x76:('ROR','dpx'),0x77:('ADC','idly'),
0x78:('SEI','imp'),0x79:('ADC','absy'),0x7A:('PLY','imp'),0x7B:('TDC','imp'),0x7C:('JMP','iax'),0x7D:('ADC','absx'),0x7E:('ROR','absx'),0x7F:('ADC','abslx'),
0x80:('BRA','rel8'),0x81:('STA','idx'),0x82:('BRL','rel16'),0x83:('STA','sr'),0x84:('STY','dp'),0x85:('STA','dp'),0x86:('STX','dp'),0x87:('STA','idl'),
0x88:('DEY','imp'),0x89:('BIT','imm_m'),0x8A:('TXA','imp'),0x8B:('PHB','imp'),0x8C:('STY','abs'),0x8D:('STA','abs'),0x8E:('STX','abs'),0x8F:('STA','absl'),
0x90:('BCC','rel8'),0x91:('STA','idy'),0x92:('STA','idp'),0x93:('STA','sry'),0x94:('STY','dpx'),0x95:('STA','dpx'),0x96:('STX','dpy'),0x97:('STA','idly'),
0x98:('TYA','imp'),0x99:('STA','absy'),0x9A:('TXS','imp'),0x9B:('TXY','imp'),0x9C:('STZ','abs'),0x9D:('STA','absx'),0x9E:('STZ','absx'),0x9F:('STA','abslx'),
0xA0:('LDY','imm_x'),0xA1:('LDA','idx'),0xA2:('LDX','imm_x'),0xA3:('LDA','sr'),0xA4:('LDY','dp'),0xA5:('LDA','dp'),0xA6:('LDX','dp'),0xA7:('LDA','idl'),
0xA8:('TAY','imp'),0xA9:('LDA','imm_m'),0xAA:('TAX','imp'),0xAB:('PLB','imp'),0xAC:('LDY','abs'),0xAD:('LDA','abs'),0xAE:('LDX','abs'),0xAF:('LDA','absl'),
0xB0:('BCS','rel8'),0xB1:('LDA','idy'),0xB2:('LDA','idp'),0xB3:('LDA','sry'),0xB4:('LDY','dpx'),0xB5:('LDA','dpx'),0xB6:('LDX','dpy'),0xB7:('LDA','idly'),
0xB8:('CLV','imp'),0xB9:('LDA','absy'),0xBA:('TSX','imp'),0xBB:('TYX','imp'),0xBC:('LDY','absx'),0xBD:('LDA','absx'),0xBE:('LDX','absy'),0xBF:('LDA','abslx'),
0xC0:('CPY','imm_x'),0xC1:('CMP','idx'),0xC2:('REP','imm8'),0xC3:('CMP','sr'),0xC4:('CPY','dp'),0xC5:('CMP','dp'),0xC6:('DEC','dp'),0xC7:('CMP','idl'),
0xC8:('INY','imp'),0xC9:('CMP','imm_m'),0xCA:('DEX','imp'),0xCB:('WAI','imp'),0xCC:('CPY','abs'),0xCD:('CMP','abs'),0xCE:('DEC','abs'),0xCF:('CMP','absl'),
0xD0:('BNE','rel8'),0xD1:('CMP','idy'),0xD2:('CMP','idp'),0xD3:('CMP','sry'),0xD4:('PEI','dp'),0xD5:('CMP','dpx'),0xD6:('DEC','dpx'),0xD7:('CMP','idly'),
0xD8:('CLD','imp'),0xD9:('CMP','absy'),0xDA:('PHX','imp'),0xDB:('STP','imp'),0xDC:('JML','ial'),0xDD:('CMP','absx'),0xDE:('DEC','absx'),0xDF:('CMP','abslx'),
0xE0:('CPX','imm_x'),0xE1:('SBC','idx'),0xE2:('SEP','imm8'),0xE3:('SBC','sr'),0xE4:('CPX','dp'),0xE5:('SBC','dp'),0xE6:('INC','dp'),0xE7:('SBC','idl'),
0xE8:('INX','imp'),0xE9:('SBC','imm_m'),0xEA:('NOP','imp'),0xEB:('XBA','imp'),0xEC:('CPX','abs'),0xED:('SBC','abs'),0xEE:('INC','abs'),0xEF:('SBC','absl'),
0xF0:('BEQ','rel8'),0xF1:('SBC','idy'),0xF2:('SBC','idp'),0xF3:('SBC','sry'),0xF4:('PEA','abs'),0xF5:('SBC','dpx'),0xF6:('INC','dpx'),0xF7:('SBC','idly'),
0xF8:('SED','imp'),0xF9:('SBC','absy'),0xFA:('PLX','imp'),0xFB:('XCE','imp'),0xFC:('JSR','iax'),0xFD:('SBC','absx'),0xFE:('INC','absx'),0xFF:('SBC','abslx'),
}

def fmt(mode, operand, pc):
    if mode in ('imm_m','imm_x','imm8'): return f"#${operand:0{ (4 if operand>0xff else 2) }x}"
    if mode=='dp': return f"${operand:02x}"
    if mode=='dpx': return f"${operand:02x},x"
    if mode=='dpy': return f"${operand:02x},y"
    if mode=='idp': return f"(${operand:02x})"
    if mode=='idx': return f"(${operand:02x},x)"
    if mode=='idy': return f"(${operand:02x}),y"
    if mode=='idl': return f"[${operand:02x}]"
    if mode=='idly': return f"[${operand:02x}],y"
    if mode=='sr': return f"${operand:02x},s"
    if mode=='sry': return f"(${operand:02x},s),y"
    if mode=='abs': return f"${operand:04x}"
    if mode=='absx': return f"${operand:04x},x"
    if mode=='absy': return f"${operand:04x},y"
    if mode=='ind': return f"(${operand:04x})"
    if mode=='iax': return f"(${operand:04x},x)"
    if mode=='ial': return f"[${operand:04x}]"
    if mode=='absl': return f"${operand:06x}"
    if mode=='abslx': return f"${operand:06x},x"
    if mode in('rel8','rel16'): return f"${operand:04x}"
    if mode=='bm': return f"${operand>>8:02x},${operand&0xff:02x}"
    return ''

def disasm(code, start, count, base=0, mx=0x30):
    """mx: P bits — 0x20=M(A 8-bit), 0x10=X(index 8-bit)."""
    pc = start; out = []
    for _ in range(count):
        if pc >= len(code): break
        op = code[pc]; mn, mode = OPC[op]
        L = M[mode]
        if L == 'M': L = 1 if (mx & 0x20) else 2
        elif L == 'X': L = 1 if (mx & 0x10) else 2
        raw = code[pc:pc+1+L]
        operand = 0
        if L:
            operand = int.from_bytes(code[pc+1:pc+1+L], 'little')
        if mode == 'rel8':
            operand = (pc + 2 + ((operand ^ 0x80) - 0x80)) & 0xffff
        elif mode == 'rel16':
            operand = (pc + 3 + ((operand ^ 0x8000) - 0x8000)) & 0xffff
        txt = f"{mn} {fmt(mode, operand, pc)}".strip()
        out.append((pc, raw, txt, mn, mode, operand))
        # 旗標追蹤
        if mn == 'REP': mx &= ~operand
        elif mn == 'SEP': mx |= operand
        pc += 1 + L
    return out

if __name__ == '__main__':
    code = open(sys.argv[1], 'rb').read()
    start = int(sys.argv[2], 16); count = int(sys.argv[3])
    mx = int(sys.argv[4], 16) if len(sys.argv) > 4 else 0x30
    for pc, raw, txt, mn, mode, opd in disasm(code, start, count, mx=mx):
        print(f"{pc:06x}: {raw.hex():14s} {txt}")
