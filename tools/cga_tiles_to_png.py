#!/usr/bin/env python3
"""把 DOS Ultima I overworld tileset(CGA Linear BIN)解碼成 832×16 PNG sprite sheet。

CGA 格式(見 src/common/graphics/CGALinearDecodeStrategy.cpp):
  2bpp linear,每 byte = 4 像素(MSB first:bit7-6=px1, 5-4=px2, 3-2=px3, 1-0=px4)。
  64 bytes/tile(16×16),52 tiles = 3328 bytes。
  palette:00 黑、01 淺青(0x55FFFF)、10 淺洋紅(0xFF55FF)、11 白。
sprite(slot>=10:玩家/載具/怪物)黑底→透明,疊在地形上。
用法:cga_tiles_to_png.py <CGATILES.BIN> <out.png>
"""
import sys
from PIL import Image

PAL = {0b00: (0, 0, 0), 0b01: (0x55, 0xFF, 0xFF), 0b10: (0xFF, 0x55, 0xFF), 0b11: (0xFF, 0xFF, 0xFF)}

def decode_tile(b):
    im = Image.new('RGBA', (16, 16))
    px = im.load()
    i = 0
    for y in range(16):
        for xb in range(4):              # 4 bytes/row,每 byte 4 px
            byte = b[i]; i += 1
            for k, shift in enumerate((6, 4, 2, 0)):
                c = (byte >> shift) & 0b11
                r, g, bl = PAL[c]
                px[xb*4 + k, y] = (r, g, bl, 255)
    return im

def main():
    binf, out = sys.argv[1], sys.argv[2]
    data = open(binf, 'rb').read()
    nt = len(data) // 64
    grid = Image.new('RGBA', (16*nt, 16), (0, 0, 0, 0))
    for t in range(nt):
        tile = decode_tile(data[t*64:t*64+64])
        if t >= 10:                       # sprite 黑底→透明
            p = tile.load()
            for y in range(16):
                for x in range(16):
                    r, g, b, a = p[x, y]
                    if r == 0 and g == 0 and b == 0:
                        p[x, y] = (0, 0, 0, 0)
        grid.paste(tile, (t*16, 0), tile)
    grid.save(out)
    print("wrote", out, nt, "CGA tiles")

if __name__ == '__main__':
    main()
