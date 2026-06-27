#!/usr/bin/env python3
"""lzss_decode.py — 解 ULTIMAI type 0x0001 resource 的 LZSS 壓縮。
演算法反組譯自 converter @0x398(見 docs/re/iigs-65816-re.md Step 6):
  控制 byte 8 bits(LSB first):bit=1 → literal(1 byte);bit=0 → match。
  match = 讀 16-bit descriptor:offset12 = desc & 0x0FFF;distance = 0x1000 - offset12;
          length = (desc >> 12) + 3;從 out[-distance] 回拷 length bytes。
用法:lzss_decode.py <resource.bin> <out.bin> [src_off] [out_size]
"""
import sys

def lzss(data, src=0, out_size=None):
    out = bytearray()
    i = src
    n = len(data)
    while i < n:
        if out_size is not None and len(out) >= out_size:
            break
        ctrl = data[i]; i += 1
        for bit in range(8):
            if out_size is not None and len(out) >= out_size:
                break
            if i >= n:
                break
            if ctrl & (1 << bit):           # bit=1 → literal
                out.append(data[i]); i += 1
            else:                            # bit=0 → match
                if i + 1 >= n: break
                desc = data[i] | (data[i+1] << 8); i += 2
                offset12 = desc & 0x0FFF
                distance = 0x1000 - offset12
                length = (desc >> 12) + 3
                start = len(out) - distance
                for k in range(length):
                    out.append(out[start + k] if 0 <= start + k < len(out) else 0)
    return bytes(out)

if __name__ == '__main__':
    data = open(sys.argv[1], 'rb').read()
    src = int(sys.argv[3], 0) if len(sys.argv) > 3 else 0
    osz = int(sys.argv[4], 0) if len(sys.argv) > 4 else None
    out = lzss(data, src, osz)
    open(sys.argv[2], 'wb').write(out)
    print(f"{sys.argv[1]}: src@{src} → {len(out)} bytes decompressed")
