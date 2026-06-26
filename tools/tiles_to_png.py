#!/usr/bin/env python3
"""把 DOS Ultima I overworld tileset(EGA RowPlanar BIN)解碼成 PNG sprite sheet。

輸出 = 832×16 水平長條(52 格 × 16×16),排列對齊 open_ultima 的 overworldTexture,
可直接當 PNG AssetPack 載入(見 SpriteSheetLoader::loadTextureFromPng)。

EGA 格式(見 src/.../EGARowPlanarDecodeStrategy.cpp):
  - 16×16/格,128 byte/格;每列 8 byte = BGRI 各 2 byte(plane order Blue,Green,Red,Intensity)。
  - 每 byte 8 像素 MSB-first;paletteIndex = B + G*2 + R*4 + I*8。

用法:tiles_to_png.py <EGATILES.BIN> <out.png>
"""
import sys
from PIL import Image

# EGA-16 palette(由 EGARowPlanarDecodeStrategy::GetPixel 0xRRGGBBAA 取 RGB)
PAL = [
    (0,0,0),(0,0,168),(0,168,0),(0,168,168),(168,0,0),(168,0,168),(168,84,0),(168,168,168),
    (84,84,84),(84,84,254),(84,254,84),(84,254,254),(254,84,84),(254,84,254),(254,254,84),(254,254,254),
]
TILE = 16
COUNT = 52
BYTES_PER_TILE = 128   # (16/8)*4planes*16rows

def decode_tile(b):
    """128 byte → 16×16 palette-index 陣列(row-major)。"""
    px = [0]*(TILE*TILE)
    for row in range(TILE):
        base = row*8
        planes = []
        for p in range(4):                 # B,G,R,I 各 2 byte
            bits = []
            for k in range(2):
                byte = b[base + p*2 + k]
                for bit in range(8):
                    bits.append((byte >> (7-bit)) & 1)
            planes.append(bits)            # 16 bits/plane/row
        for x in range(TILE):
            idx = planes[0][x] + planes[1][x]*2 + planes[2][x]*4 + planes[3][x]*8
            px[row*TILE + x] = idx
    return px

def main():
    binf, out = sys.argv[1], sys.argv[2]
    data = open(binf, "rb").read()
    im = Image.new("RGB", (TILE*COUNT, TILE), (0,0,0))
    for t in range(COUNT):
        tb = data[t*BYTES_PER_TILE:(t+1)*BYTES_PER_TILE]
        if len(tb) < BYTES_PER_TILE: break
        px = decode_tile(tb)
        for y in range(TILE):
            for x in range(TILE):
                im.putpixel((t*TILE + x, y), PAL[px[y*TILE + x]])
    im.save(out)
    print(f"wrote {out}  ({im.width}x{im.height}, {COUNT} tiles)")

if __name__ == "__main__":
    main()
