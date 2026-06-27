#!/usr/bin/env python3
"""把 DOS Ultima I 的 Tandy(T1K)overworld tileset 解碼成 PNG sprite sheet。

輸出 = 832×16 水平長條(52 格 × 16×16),對齊 open_ultima overworldTexture,
可直接當 PNG AssetPack 載入,與 ega/cga/vga/iigs/msx/fmtowns 並列熱鍵切換。

Tandy(T1K)格式 — 與 EGA 同檔長(6656B=52×128)但**佈局不同**:
  - EGA 是 RowPlanar(4 bitplane 交錯,每像素跨 4 個 byte 的同位 bit)。
  - **Tandy 是 chunky / packed-pixel 4bpp**:每 byte = 2 像素(高 nibble = 左像素、低 nibble = 右像素),
    8 byte/列 × 2 = 16 像素/列,16 列 = 128 byte/格。這是 Tandy 1000 / PCjr 16 色模式(TGA)的記憶體佈局。
  - palette 沿用標準 16 色(同 EGA default);Tandy TGA 16 色與 EGA 同調色盤。
驗證:tile0 解出 = 水(亮藍 idx9 點狀波紋,逐列右移 4px);結構與 ega.png 一致。

用法:t1k_tiles_to_png.py <T1KTILES.BIN> <out.png>
"""
import sys
from PIL import Image

# 標準 16 色 palette(同 EGARowPlanarDecodeStrategy::GetPixel)
PAL = [
    (0,0,0),(0,0,168),(0,168,0),(0,168,168),(168,0,0),(168,0,168),(168,84,0),(168,168,168),
    (84,84,84),(84,84,254),(84,254,84),(84,254,254),(254,84,84),(254,84,254),(254,254,84),(254,254,254),
]
TILE = 16
COUNT = 52
BYTES_PER_TILE = 128   # 8 byte/列 × 16 列(chunky 4bpp,2px/byte)

def decode_tile(b):
    """128 byte → 16×16 palette-index 陣列(row-major,chunky 4bpp 高 nibble 左)。"""
    px = [0]*(TILE*TILE)
    for row in range(TILE):
        base = row*8                       # 每列 8 byte
        for col in range(8):
            byte = b[base + col]
            px[row*TILE + col*2]   = (byte >> 4) & 15   # 高 nibble = 左像素
            px[row*TILE + col*2+1] = byte & 15          # 低 nibble = 右像素
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
