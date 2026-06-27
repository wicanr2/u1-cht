#!/usr/bin/env python3
"""omf_parse.py — 解析 Apple IIgs GS/OS OMF v2 可執行檔的段表。
見 docs/re/iigs-65816-re.md Step 1。用法:omf_parse.py <ULTIMAI>
"""
import sys, struct

KIND = {0: 'CODE', 1: 'DATA', 0x8000: 'CODE(dyn)', 0x8001: 'DATA(dyn)'}

def parse_seg(d, off):
    h = d[off:]
    bytecnt, resspc, length = struct.unpack('<III', h[0:12])
    lablen, numlen, version = h[13], h[14], h[15]
    banksize, = struct.unpack('<I', h[16:20])
    kind, = struct.unpack('<H', h[20:22])
    org, = struct.unpack('<I', h[24:28])
    segnum, = struct.unpack('<H', h[34:36])
    entry, = struct.unpack('<I', h[36:40])
    dispname, dispdata = struct.unpack('<HH', h[40:44])
    loadname = h[44:54].rstrip(b'\x00 ')
    # SEGNAME:dispname 處,1-byte 長度前綴(LABLEN=0 → variable)
    slen = h[dispname]; segname = h[dispname+1:dispname+1+slen]
    return dict(off=off, bytecnt=bytecnt, length=length, kind=kind, version=version,
                org=org, segnum=segnum, entry=entry, dispdata=dispdata,
                loadname=loadname, segname=segname)

def segments(d):
    off = 0; out = []
    while off < len(d):
        s = parse_seg(d, off)
        if s['bytecnt'] == 0: break
        out.append(s); off += s['bytecnt']
    return out

if __name__ == '__main__':
    d = open(sys.argv[1], 'rb').read()
    print(f"total {len(d)} bytes")
    for s in segments(d):
        print(f"seg#{s['segnum']} @0x{s['off']:05x} bytecnt={s['bytecnt']} "
              f"mem_len={s['length']} kind=0x{s['kind']:04x}({KIND.get(s['kind'],'?')}) "
              f"dispdata={s['dispdata']} entry=0x{s['entry']:x} load={s['loadname']}")
