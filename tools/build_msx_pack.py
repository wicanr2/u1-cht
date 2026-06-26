#!/usr/bin/env python3
"""組 MSX U1 PNG AssetPack(832×16,對齊 engine 52 槽)。

來源:MSXTILES.BIN(96 tiles,MSX 原生順序)— 格式 + 解碼見 docs/re/e3-msx.md「Path B」。
SLOT_MAP:engine slot(見 src/overworld/TileTypeLoader.cpp 順序)→ MSX tile index(0..95)。
地形/結構/船/海怪/玩家為高信心對映;陸上怪物/NPC 因 MSX 與 DOS sprite 集不同,屬最佳近似。
用法:build_msx_pack.py <MSXTILES.BIN> <out.png>
"""
import sys
from PIL import Image

# V9938 8 色 GRB palette,index = G<<2|R<<1|B(實測,見 e3-msx.md)
PAL = [(0,0,0),(0,0,255),(255,0,0),(255,0,255),(0,255,0),(0,255,255),(255,255,0),(255,255,255)] + [(0,0,0)]*8

# engine slot 註解(0-48 用;49-51 額外)。值 = MSX tile index。
# 高信心:0-1 地形,3 森林,26 山,6-7 城堡,9/11 城鎮,14 地牢,50 玩家,
#         24 筏,20 frigate,27 梭,28 時光機,29 海蛇,32 烏賊,35 龍,38 海盜船。
SLOT_MAP = [
    0,    # 0  WATER
    1,    # 1  GRASS
    3,    # 2  FOREST
    26,   # 3  MOUNTAIN
    6,    # 4  CASTLE
    7,    # 5  CASTLE(flag swap)
    13,   # 6  SIGNPOST(無專屬→用要塞,近似)
    9,    # 7  TOWN
    11,   # 8  TOWN(swap)
    14,   # 9  DUNGEON_ENTRANCE(黑底拱門,強匹配)
    50,   # 10 PLAYER(持劍騎士)
    17,   # 11 HORSE(黃色橫向坐騎,近似)
    18,   # 12 CART(無專屬→用小型載具,近似)
    24,   # 13 RAFT(平底藍船)
    20,   # 14 FRIGATE(有桅帆船)
    21,   # 15 FRIGATE(swap)
    23,   # 16 AIRCAR(載具,近似)
    27,   # 17 SHUTTLE(火箭/梭)
    28,   # 18 TIME_MACHINE(圓形裝置)
    29,   # 19 NESS_MONSTER(海蛇)
    30,   # 20 swap
    32,   # 21 GIANT_SQUID(烏賊)
    33,   # 22 swap
    35,   # 23 DRAGON_TURTLE(綠龍)
    36,   # 24 swap
    38,   # 25 PIRATE_SHIP(白桅海盜船)
    39,   # 26 swap
    40,   # 27 HOOD(人形)
    41,   # 28 swap
    53,   # 29 BEAR(綠獸,近似)
    54,   # 30 swap
    84,   # 31 HIDDEN_ARCHER(持劍衛兵,近似)
    86,   # 32 swap
    62,   # 33 DARK_KNIGHT(騎士)
    63,   # 34 swap
    55,   # 35 EVIL_TRENT(綠獸)
    53,   # 36 swap
    56,   # 37 THIEF(戰士)
    57,   # 38 swap
    54,   # 39 ORC(綠獸)
    55,   # 40 swap
    49,   # 41 KNIGHT(持劍騎士)
    51,   # 42 swap
    71,   # 43 NECROMANCER(持杖法師)
    70,   # 44 swap
    64,   # 45 EVIL_RANGER(持劍戰士)
    65,   # 46 swap
    72,   # 47 WANDERING_WARLOCK(紫甲)
    73,   # 48 swap(魔法特效)
    74,   # 49 (額外:特效)
    75,   # 50 (額外:特效)
    76,   # 51 (額外:小怪)
]

def load_tiles(path):
    raw = open(path, 'rb').read()
    body = raw[4:]
    N = 192  # bytes/line for MSXTILES
    def tile(idx):
        col = idx % 24; row = idx // 24
        im = Image.new('RGB', (16, 16))
        for ty in range(16):
            for tx in range(16):
                x = col*16 + tx
                b = body[(row*16+ty)*N + (x >> 1)]
                n = (b >> 4) if (x & 1) == 0 else (b & 0xF)
                im.putpixel((tx, ty), PAL[n & 15])
        return im
    return tile

def main():
    src, out = sys.argv[1], sys.argv[2]
    tile = load_tiles(src)
    grid = Image.new('RGBA', (16*52, 16), (0,0,0,0))
    for slot in range(52):
        t = tile(SLOT_MAP[slot]).convert('RGBA')
        if slot >= 10:  # 玩家/載具/怪物:黑底→透明(疊在地形上)
            px = t.load()
            for y in range(16):
                for x in range(16):
                    r, g, b, a = px[x, y]
                    if r == 0 and g == 0 and b == 0:
                        px[x, y] = (0, 0, 0, 0)
        grid.paste(t, (slot*16, 0), t)
    grid.save(out)
    print("wrote", out, "52 slots from MSX tiles", SLOT_MAP)

if __name__ == '__main__':
    main()
