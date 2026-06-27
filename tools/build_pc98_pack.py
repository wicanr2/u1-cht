#!/usr/bin/env python3
"""組 PC-98 U1 PNG AssetPack(832×16,對齊 engine 52 槽)。

來源:PC-98 版《Ultima I》磁碟內的 **EGCTILES.BIN**(30720B)。
格式由自相關 + 結構比對破解:**60 個 32×32 tile,4-plane planar,plane-major,512B/tile**
(每 plane = 32 列 × 4 byte;像素 index = p0 + p1·2 + p2·4 + p3·8)。EGC = PC-98 16 色圖形硬體。
palette 由 hg101 PC-98 實機截圖反推:全 8 個出現的 index 完美符合 **R=b2, G=b3, B=b1**(×255),
b0 不影響色(palette 有重複項 → 實質 8 純 RGB 色)。背景 = 黑(index 0/1)。

engine 槽序見 src/overworld/TileTypeLoader.cpp(權威)。PC-98 tile 順序與 engine 幾乎一致,
唯 **engine slot 10=PLAYER 在 PC-98 無獨立 tile**(dungeon 9 後直接 horse 10)→ player 取尾段 tile 54
(白袍黃髮持鞭,與 PC98-02 overworld 截圖玩家吻合);horse 之後 pc98 index = engine slot − 1。

PC-98 是 32×32 高解析,engine tile 為 16×16 → 以 **2×2 block 取眾數**降採樣(index 空間,保色純)。

用法:build_pc98_pack.py <EGCTILES.BIN> <out.png>
"""
import sys
from collections import Counter
from PIL import Image

T = 32                       # PC-98 tile 邊長
PLANE = T * 4                # 每 plane 32 列 × 4 byte = 128
BPT = PLANE * 4              # 512 byte/tile(4 plane)
OUT = 16                     # engine tile 邊長
PAL = [(255*((i >> 2) & 1), 255*((i >> 3) & 1), 255*((i >> 1) & 1)) for i in range(16)]

# engine slot → PC-98 EGCTILES tile index(見 docstring 與 TileTypeLoader.cpp)
SLOT_MAP = [
    0, 1, 2, 3,          # 0 water 1 grass 2 forest 3 mountain
    4, 5,                # 4-5 castle(雙幀)
    6,                   # 6 signpost
    7, 8,                # 7-8 town(雙幀)
    9,                   # 9 dungeon
    54,                  # 10 player(尾段白袍持鞭,PC-98 無獨立 player 於主序)
    10, 11, 12,          # 11 horse 12 cart 13 raft
    13, 14,              # 14-15 frigate
    15, 16, 17,          # 16 aircar 17 shuttle 18 time machine
    18, 19,              # 19-20 ness
    20, 21,              # 21-22 squid
    22, 23,              # 23-24 dragon turtle
    24, 25,              # 25-26 pirate ship
    26, 27,              # 27-28 hood
    28, 29,              # 29-30 bear
    30, 31,              # 31-32 hidden archer
    32, 33,              # 33-34 dark knight
    34, 35,              # 35-36 evil trent
    36, 37,              # 37-38 thief
    38, 39,              # 39-40 orc
    40, 41,              # 41-42 knight
    42, 43,              # 43-44 necromancer
    44, 45,              # 45-46 evil ranger
    46, 47,              # 47-48 wandering warlock
    48, 49, 50,          # 49-51 額外(engine 未用)
]

def decode_tile(tb):
    """512 byte → 32×32 palette-index(row-major,4-plane plane-major)。"""
    px = [0]*(T*T)
    for row in range(T):
        planes = []
        for p in range(4):
            base = p*PLANE + row*4
            bits = []
            for k in range(4):
                by = tb[base+k]
                for b in range(8):
                    bits.append((by >> (7-b)) & 1)
            planes.append(bits)
        for x in range(T):
            px[row*T+x] = planes[0][x] + planes[1][x]*2 + planes[2][x]*4 + planes[3][x]*8
    return px

def downscale(px):
    """32×32 index → 16×16,2×2 block 取眾數(平手偏非黑前景,保 sprite 細節)。"""
    out = [0]*(OUT*OUT)
    for oy in range(OUT):
        for ox in range(OUT):
            blk = [px[(oy*2+dy)*T + ox*2+dx] for dy in (0, 1) for dx in (0, 1)]
            c = Counter(blk)
            top = c.most_common()
            best_n = top[0][1]
            cands = [v for v, n in top if n == best_n]
            # 平手:偏非黑(index 的 RGB 非全 0)前景
            nonblack = [v for v in cands if PAL[v] != (0, 0, 0)]
            out[oy*OUT+ox] = (nonblack or cands)[0]
    return out

def main():
    binf, outp = sys.argv[1], sys.argv[2]
    data = open(binf, "rb").read()
    tiles = [downscale(decode_tile(data[t*BPT:(t+1)*BPT])) for t in range(len(data)//BPT)]
    grid = Image.new("RGBA", (OUT*52, OUT), (0, 0, 0, 0))
    for slot in range(52):
        px = tiles[SLOT_MAP[slot]]
        tile = Image.new("RGBA", (OUT, OUT)); p = tile.load()
        for y in range(OUT):
            for x in range(OUT):
                r, g, b = PAL[px[y*OUT+x]]
                # sprite(slot>=10)的黑底 → 透明
                a = 0 if (slot >= 10 and (r, g, b) == (0, 0, 0)) else 255
                p[x, y] = (r, g, b, a)
        grid.paste(tile, (slot*OUT, 0), tile)
    grid.save(outp)
    print(f"wrote {outp} — 52 槽 PC-98 tile(EGCTILES 32×32→16×16,4-plane planar)")

if __name__ == "__main__":
    main()
