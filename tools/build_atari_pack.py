#!/usr/bin/env python3
"""組 Atari 8-bit U1 PNG AssetPack(832×16,對齊 engine 52 槽)。

來源:Atari 8-bit《Ultima I》(1983)overworld 程式 `OUTMOVE.bin`(Atari binary load 格式)。
tile 格式由 RE 破解(見 docs/re/atari-tileset-re.md):
  - **charset @$6400,1bpp(每 byte 8 像素,1 byte/列)** —— 非 2bpp(這是先前卡關主因)。
  - 一個 overworld tile = **8 邏輯寬 × 16 高 = 上半 char[i] + 下半 char[i+32]**(兩段 charset 即上/下半)。
  - 顯示時水平 2×(8 邏輯 px → 16 px),共 **19 個 tile**(Atari 版比 DOS 52 精簡)。
驗證:用實機圖 atari800-04 反推,grass=(char1,char33)、forest=(2,34)、player=(8,40),規律 = 上 i / 下 i+32。
palette:截圖實證 overworld 僅 3 前景色 —— 水=藍、植被(草/林/山)=橄欖綠、其餘(結構/玩家/生物)=灰,黑底。

19→52 槽:Atari tile 依語意對到 engine 槽;engine 多出的槽(動畫雙幀、DOS 才有的載具/怪物)
複用最接近的 Atari tile。詳見 SLOT_MAP。

用法:build_atari_pack.py <OUTMOVE.bin> <out.png>
"""
import sys
from PIL import Image

CHBASE = 0x6400
OLIVE = (150, 176, 0)
BLUE = (40, 120, 255)
GRAY = (190, 190, 190)

# 各 Atari tile 的前景色(0 水/1-3 植被/其餘 結構·生物)
def tile_color(t):
    if t == 0:
        return BLUE
    if t in (1, 2, 3):
        return OLIVE
    return GRAY

# engine slot(52)→ Atari tile(0-18)。語意對應;雙幀/缺項複用最近 tile。
# Atari: 0水 1草 2林 3山 4城堡 5城堡(旗) 6城鎮 7地牢 8玩家 9馬
#        10筏/船 11路標/柱 12帆船 13載具 14梭/火箭 15生物 16蛇 17生物 18生物
SLOT_MAP = [
    0,            # 0 water
    1,            # 1 grass
    2,            # 2 forest
    3,            # 3 mountain
    4, 5,         # 4-5 castle(雙幀:城堡 + 帶旗城堡)
    11,           # 6 signpost
    6, 6,         # 7-8 town
    7,            # 9 dungeon
    8,            # 10 player
    9,            # 11 horse
    10,           # 12 cart(複用 筏/船)
    10,           # 13 raft
    12, 12,       # 14-15 frigate
    13,           # 16 aircar
    14,           # 17 shuttle
    14,           # 18 time machine(複用)
    15, 15,       # 19-20 ness
    16, 16,       # 21-22 squid(蛇形)
    17, 17,       # 23-24 dragon
    18, 18,       # 25-26 pirate ship(複用生物)
    15, 15,       # 27-28 hood
    16, 16,       # 29-30 bear
    17, 17,       # 31-32 archer
    18, 18,       # 33-34 dark knight
    15, 15,       # 35-36 evil trent
    16, 16,       # 37-38 thief
    17, 17,       # 39-40 orc
    18, 18,       # 41-42 knight
    15, 15,       # 43-44 necromancer
    16, 16,       # 45-46 evil ranger
    17, 17,       # 47-48 wandering warlock
    18, 18, 18,   # 49-51 額外
]

def load_mem(path):
    d = open(path, "rb").read()
    i = 2 if d[:2] == b"\xff\xff" else 0
    mem = bytearray(65536)
    while i + 4 <= len(d):
        if d[i:i+2] == b"\xff\xff":
            i += 2; continue
        s = d[i] | (d[i+1] << 8); e = d[i+2] | (d[i+3] << 8); i += 4
        n = e - s + 1
        if n <= 0 or i + n > len(d):
            break
        mem[s:s+n] = d[i:i+n]; i += n
    return mem

def main():
    binf, outp = sys.argv[1], sys.argv[2]
    mem = load_mem(binf)
    grid = Image.new("RGBA", (16 * 52, 16), (0, 0, 0, 0))
    for slot in range(52):
        t = SLOT_MAP[slot]
        col = tile_color(t)
        tile = Image.new("RGBA", (16, 16), (0, 0, 0, 0)); px = tile.load()
        for y in range(8):
            top = mem[CHBASE + t*8 + y]
            bot = mem[CHBASE + (t+32)*8 + y]
            for x in range(8):
                for half, byte in ((0, top), (8, bot)):
                    if (byte >> (7-x)) & 1:
                        # 8 邏輯 px → 16 顯示(水平 2×)
                        px[x*2, half + y] = (*col, 255)
                        px[x*2+1, half + y] = (*col, 255)
        # 地形(slot 0-3)黑底不透明;sprite(slot>=4 的結構/生物)黑底透明
        if slot <= 3:
            bg = Image.new("RGBA", (16, 16), (0, 0, 0, 255))
            bg.paste(tile, (0, 0), tile); tile = bg
        grid.paste(tile, (slot*16, 0), tile)
    grid.save(outp)
    print(f"wrote {outp} — 19 個真實 Atari tile(OUTMOVE $6400 1bpp,char i + i+32)對映 52 槽")

if __name__ == "__main__":
    main()
