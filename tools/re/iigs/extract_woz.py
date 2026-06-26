#!/usr/bin/env python3
"""extract_woz.py — 從 Apple IIgs WOZ 磁碟映像抽出檔案 + 解析 GS/OS resource fork。

E2 woz「牆」的解法(見 docs/re/e2-apple-iigs.md):
  woz 是 flux/bit-stream(Applesauce),3.5" 用 GCR 6-and-2 編碼。
  ★ 不必自寫 GCR 解碼器 —— **MAME `floptool`**(docker image `u1-a2`,apt mame-tools)內建
    WOZ↔ProDOS 編解碼:
      floptool flopdir  woz prodos <woz>                 # 列目錄
      floptool flopread woz prodos <woz> <path> <out>    # 抽檔(data fork → out;rsrc fork → ._out AppleDouble)
  本檔負責第 2 步之後:解 AppleDouble 取 resource fork、解析 IIgs Resource Manager map、dump 各 resource。

IIgs 4bpp 像素格式(已用 rIcon 驗證):row-major,ceil(w/2) bytes/row,高 nibble = 左像素。
type 0x8001 rIcon = [type,size,h,w](各 word)+ image[h*ceil(w/2)] + mask。
大宗圖像 resource(type 0x0001 / 0x8024)為 **Apple PackBytes 壓縮**(從 byte 0 起;見 unpackbytes())。
type 0x8015 = 遊戲文字字串(翻譯用)。

用法:extract_woz.py <ULTIMAI.rsrc 或 ._ULTIMAI> <out_dir>
"""
import sys, struct, os

def unpackbytes(src):
    """Apple IIgs PackBytes 解壓(Miscellaneous Tool Set UnPackBytes 演算法)。"""
    out = bytearray(); i = 0; n = len(src)
    while i < n:
        f = src[i]; i += 1
        t = f >> 6; c = (f & 0x3F) + 1
        if t == 0:                      # 0x00-0x3F:c 個 literal byte
            out += src[i:i+c]; i += c
        elif t == 1:                    # 0x40-0x7F:單 byte 重複 c 次
            out += bytes([src[i]]) * c; i += 1
        elif t == 2:                    # 0x80-0xBF:4-byte group 重複 c 次
            out += src[i:i+4] * c; i += 4
        else:                           # 0xC0-0xFF:單 byte 重複 c*4 次
            out += bytes([src[i]]) * (c*4); i += 1
    return bytes(out)

def appledouble_rsrc(path):
    """從 ._xxx AppleDouble 取出 resource fork(entry id=2)。
    floptool 產生的 ._ header entry 偏移不標準 → 用『檔尾 = rsrc fork』後援。"""
    d = open(path, 'rb').read()
    if d[:4] != b'\x00\x05\x16\x07':
        return d  # 已是裸 rsrc fork
    # floptool 的 ._ header entry 偏移不標準 → 掃描候選 offset,選「IIgs map 能成功解析」者。
    for off in range(0, min(len(d), 512)):
        cand = d[off:]
        if len(cand) < 32:
            break
        ver, tomap, msz = struct.unpack('<III', cand[:12])
        if not (ver < 16 and 0 < tomap < len(cand) - 32 and 0 < msz < len(cand)):
            continue
        try:
            if len(parse_resources(cand)) >= 4:
                return cand
        except (struct.error, IndexError):
            continue
    return d

def parse_resources(res):
    ver, tomap, mapsize = struct.unpack('<III', res[:12])
    m = tomap
    (mapNext, mapFlag, mapOffset, mapSize, mapToIndex, mapFileNum, mapID,
     mapIndexSize, mapIndexUsed, mapFreeListSize, mapFreeListUsed) = struct.unpack('<IHIIHHHIIHH', res[m:m+32])
    idx = m + mapToIndex
    recs = []
    for i in range(mapIndexUsed):
        rt, rid, roff, ratt, rsz, rhnd = struct.unpack('<HIIHII', res[idx+i*20:idx+i*20+20])
        if rt == 0 and rsz == 0:
            continue
        recs.append((rt, rid, roff, rsz))
    return recs

def main():
    src, outdir = sys.argv[1], sys.argv[2]
    os.makedirs(outdir, exist_ok=True)
    res = appledouble_rsrc(src)
    open(os.path.join(outdir, 'fork.rsrc'), 'wb').write(res)
    recs = parse_resources(res)
    from collections import defaultdict
    bytype = defaultdict(int)
    for rt, rid, roff, rsz in recs:
        open(os.path.join(outdir, f"t{rt:04x}_id{rid:02x}_{rsz}.bin"), 'wb').write(res[roff:roff+rsz])
        bytype[rt] += 1
    print(f"extracted {len(recs)} resources to {outdir}")
    for rt in sorted(bytype):
        print(f"  type 0x{rt:04x}: {bytype[rt]}")

if __name__ == '__main__':
    main()
