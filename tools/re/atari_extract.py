#!/usr/bin/env python3
"""從 Atari U1 (1983 SierraVenture) ATR 抽出 Atari DOS 2 檔案 + 解析 binary load 分段。
原版怪物 AI oracle 用。用法:atari_extract.py <side_A.atr> <outdir>
不入庫的版權碼只寫到 outdir(re_work/),本腳本記錄可重跑流程。"""
import struct, os, sys

def load_atr(path):
    d = open(path, "rb").read()
    assert d[0:2] == b'\x96\x02', "非 ATR"
    return d, 128  # single density 128-byte sectors

def sector(d, ss, n):  # 1-based
    return d[16 + (n - 1) * ss: 16 + n * ss]

def list_dir(d, ss):
    files = []
    for s in range(361, 369):           # Atari DOS 2 目錄 sector 361-368
        b = sector(d, ss, s)
        for e in range(8):
            ent = b[e * 16:(e + 1) * 16]
            if ent[0] == 0 or not (ent[0] & 0x40):
                continue
            cnt = struct.unpack('<H', ent[1:3])[0]
            start = struct.unpack('<H', ent[3:5])[0]
            name = ent[5:13].decode('latin1').strip()
            ext = ent[13:16].decode('latin1').strip()
            files.append((name, ext, start, cnt))
    return files

def extract_file(d, ss, start):         # Atari DOS 2 sector chain
    out = bytearray(); s = start
    while s:
        b = sector(d, ss, s)
        out += b[:b[127]]               # byte127 = valid byte count
        s = ((b[125] & 0x03) << 8) | b[126]   # next sector
    return bytes(out)

def parse_binary_load(data):            # FFFF, then (start,end,data) segments
    i = 2 if data[:2] == b'\xff\xff' else 0
    mem = bytearray(0x10000); segs = []; runad = None
    while i + 4 <= len(data):
        start = struct.unpack('<H', data[i:i+2])[0]
        end = struct.unpack('<H', data[i+2:i+4])[0]; i += 4
        if start == 0xFFFF:
            continue
        ln = end - start + 1
        mem[start:start+ln] = data[i:i+ln]; i += ln
        segs.append((start, end))
        if start == 0x02E0:             # RUNAD = Atari run vector
            runad = struct.unpack('<H', data[i-ln:i-ln+2])[0]
    return mem, segs, runad

if __name__ == "__main__":
    atr, outdir = sys.argv[1], sys.argv[2]
    os.makedirs(outdir, exist_ok=True)
    d, ss = load_atr(atr)
    for nm, ex, st, ct in list_dir(d, ss):
        print(f"{nm:8}.{ex:3} start={st:3} sectors={ct}")
        raw = extract_file(d, ss, st)
        open(os.path.join(outdir, f"{nm}.bin"), "wb").write(raw)
