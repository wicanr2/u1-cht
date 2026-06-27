#!/usr/bin/env python3
"""組 Apple IIgs U1 PNG AssetPack(832×16,對齊 engine 52 槽)— 完整真實版。

來源:**LZSS 解壓 type 0x0001 resource id08**,其 offset 11264 起(tile 88-135)= 48 個 16×16 overworld tile。
此格式由反組譯 ULTIMAI(65816)破解(LZSS + DrawTile @0x0f75 + $4c00 48-tile buffer,$c4df 索引);
tileset 位置由「hg101 實機截圖結構比對」定位(grass/water 1.00 命中 id08)。見 docs/re/apple-iigs-reverse-engineering.md。
palette 用 hg101 截圖 07/12 對 id08 tile 像素反推(index→RGB)。

用法:build_iigs_pack.py <id08_resource.bin> <out.png>
  id08 = re_work/iigs/res/t0001_id08_*.bin(從 woz 抽出,版權,gitignore)。
"""
import sys, glob, importlib.util
from collections import Counter
from PIL import Image

_spec = importlib.util.spec_from_file_location('lz', __file__.rsplit('/', 1)[0] + '/re/iigs/lzss_decode.py')
lz = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(lz)

TILE_BASE = 88            # id08 tile 88-135 = overworld tileset(48 tiles)
# engine slot → IIgs tile(88-135)。IIgs 順序幾乎對齊 engine;動畫雙幀對應雙 tile。
SLOT_MAP = [
    88, 89, 90, 91,       # 0 water 1 grass 2 forest 3 mountain
    92, 92,               # 4-5 castle(+swap)
    93,                   # 6 signpost
    94, 94,               # 7-8 town(+swap)
    95,                   # 9 dungeon
    96, 97, 98, 99,       # 10 player 11 horse 12 cart 13 raft
    100, 100,             # 14-15 frigate(+swap)
    101, 102, 103,        # 16 aircar 17 shuttle 18 time machine
    104, 105,             # 19-20 ness
    106, 107,             # 21-22 squid
    108, 109,             # 23-24 dragon turtle
    110, 111,             # 25-26 pirate ship
    112, 113,             # 27-28 hood
    114, 115,             # 29-30 bear
    116, 117,             # 31-32 hidden archer
    118, 119,             # 33-34 dark knight
    120, 121,             # 35-36 evil trent
    122, 123,             # 37-38 thief
    124, 125,             # 39-40 orc
    126, 127,             # 41-42 knight
    128, 129,             # 43-44 necromancer
    130, 131,             # 45-46 evil ranger
    132, 133,             # 47-48 wandering warlock
    134, 135, 135,        # 49-51 額外
]

def tile_indices(out, t):
    b = out[t*128:t*128+128]; idx = []
    for y in range(16):
        for xb in range(8):
            by = b[y*8+xb]; idx.append((by >> 4) & 15); idx.append(by & 15)
    return idx

def derive_palette(out):
    """用 hg101 截圖 07/12 對 id08 tile 反推 index→RGB(多數決)。"""
    im07 = Image.open('reference/hg101/imgs/ultima1-appleIIgs-07.png').convert('RGB')
    im12 = Image.open('reference/hg101/imgs/ultima1-appleIIgs-12.png').convert('RGB')
    def scr(im, cx, cy): return [im.getpixel((cx+x, cy+y)) for y in range(16) for x in range(16)]
    # (id08 tile, screenshot im, cell x, cell y)— offset (8,8) 網格
    pairs = [
        (89, im07, 8+16*4, 8+16*0),   # grass
        (88, im07, 8+16*0, 8+16*6),   # water
        (92, im07, 8+16*0, 8+16*2),   # castle
        (96, im07, 8+16*9, 8+16*4),   # player
        (90, im07, 8+16*2, 8+16*3),   # forest
        (91, im12, 8+16*3, 8+16*4),   # mountain
        (97, im12, 8+16*9, 8+16*1),   # horse
    ]
    votes = {i: Counter() for i in range(16)}
    for t, im, cx, cy in pairs:
        for idx, rgb in zip(tile_indices(out, t), scr(im, cx, cy)):
            votes[idx][rgb] += 1
    pal = [(0, 0, 0)] * 16
    for i in range(16):
        if votes[i]:
            pal[i] = votes[i].most_common(1)[0][0]
    return pal

def main():
    id08_path, out_png = sys.argv[1], sys.argv[2]
    out = lz.lzss(open(id08_path, 'rb').read(), 2)
    pal = derive_palette(out)
    bg = out[TILE_BASE*128]  # 背景索引(grass/water 的最常見;通常 index 0)
    grid = Image.new('RGBA', (16*52, 16), (0, 0, 0, 0))
    for slot in range(52):
        t = SLOT_MAP[slot]; b = out[t*128:t*128+128]
        tile = Image.new('RGBA', (16, 16)); p = tile.load()
        for y in range(16):
            for xb in range(8):
                by = b[y*8+xb]
                for nib, col in (((by >> 4) & 15, xb*2), (by & 15, xb*2+1)):
                    r, g, bl = pal[nib]
                    # sprite(slot>=10)的背景(index 0 = 黑)→ 透明
                    a = 0 if (slot >= 10 and nib == 0) else 255
                    p[col, y] = (r, g, bl, a)
        grid.paste(tile, (slot*16, 0), tile)
    grid.save(out_png)
    print("wrote", out_png, "— 48 個真實 IIgs overworld tile(id08 88-135,LZSS 解壓)")

if __name__ == '__main__':
    main()
