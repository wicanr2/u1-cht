#!/usr/bin/env python3
"""render_shr.py — 把 Apple IIgs Super Hi-Res 螢幕記憶體 dump 渲染成 PNG。

來源:MAME apple2gs 的 SHR 記憶體($E12000–$E19FFF,32KB)經 lua dump(見 mame_dump.lua)。
SHR 格式(IIgs):
  $0000–$7CFF  pixel data(200 lines × 160 bytes/line,4bpp chunky,高 nibble = 左像素)
  $7D00–$7DC7  SCB(Scan Line Control Byte × 200):低 nibble = palette index(0-15),bit7 = 640 mode
  $7E00–$7FFF  16 個 palette × 32 bytes(每色 12-bit `$0RGB` word,LE)
用法:render_shr.py <shr_dump.bin> <out.png>
"""
import sys
from PIL import Image

def render(path, out, scale=2):
    d = open(path, 'rb').read()
    pix = d[0:0x7D00]; scb = d[0x7D00:0x7DC8]; pal = d[0x7E00:0x8000]
    img = Image.new('RGB', (320, 200)); px = img.load()
    for y in range(200):
        s = scb[y] if y < len(scb) else 0
        pbase = (s & 0x0F) * 32
        cols = []
        for c in range(16):
            w = pal[pbase + c*2] | (pal[pbase + c*2 + 1] << 8)   # $0RGB word LE
            cols.append((((w >> 8) & 0xF) * 17, ((w >> 4) & 0xF) * 17, (w & 0xF) * 17))
        for xb in range(160):
            b = pix[y*160 + xb]
            px[xb*2, y] = cols[(b >> 4) & 0xF]
            px[xb*2 + 1, y] = cols[b & 0xF]
    img.resize((320*scale, 200*scale), Image.NEAREST).save(out)
    print("rendered", out)

if __name__ == '__main__':
    render(sys.argv[1], sys.argv[2])
