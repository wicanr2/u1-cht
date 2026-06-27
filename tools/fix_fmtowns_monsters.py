#!/usr/bin/env python3
"""修正 FM Towns pack 的怪物槽(19-51):原 build cycle UT1TILE0 跑進投射物(56-63)→ 雜訊。
改用明確 monster mapping(只用有效怪物 tile 0-54,避開 55 空 / 56-63 投射物)覆蓋。

UT1TILE0(`re_work/fmt/ut1tile0.png`,32×32 直條 64 格)有效怪物:
  0-1 騎士 6-7 綠獸(orc) 8-9 綠遊俠 10-11 紅綠 12-13 綠張臂 16-17 橙袍 22-23 海蛇 24-25 烏賊
  26-27 龍 28-29 海盜船 30-31 藍 32-33 綠張臂 40-41 持劍 46-47 法師 48-49 骷髏 50-51 藍法師
  52-53 綠法師 54 天使;55 空、56-63 投射物(別用)。
slot 對映(DOS 怪物順序,2-frame 對),近似但全為有效 FM Towns 怪物。
用法:fix_fmtowns_monsters.py <fmtowns.png> <ut1tile0.png> <out.png>
"""
import sys
from PIL import Image

# engine slot(19-51)→ UT1TILE0 index
SLOT_T0 = {
    19: 22, 20: 23,   # NESS(海蛇)
    21: 24, 22: 25,   # SQUID(烏賊)
    23: 26, 24: 27,   # DRAGON_TURTLE(龍)
    25: 28, 26: 29,   # PIRATE_SHIP(海盜船)
    27: 16, 28: 17,   # HOOD(橙袍)
    29: 12, 30: 13,   # BEAR→綠獸(近似)
    31: 8,  32: 9,    # HIDDEN_ARCHER(綠遊俠)
    33: 40, 34: 41,   # DARK_KNIGHT(持劍)
    35: 32, 36: 33,   # EVIL_TRENT(綠張臂)
    37: 30, 38: 31,   # THIEF(藍)
    39: 6,  40: 7,    # ORC(綠獸)
    41: 0,  42: 1,    # KNIGHT(騎士)
    43: 52, 44: 53,   # NECROMANCER(綠法師)
    45: 10, 46: 11,   # EVIL_RANGER(紅綠)
    47: 50, 48: 51,   # WANDERING_WARLOCK(藍法師)
    49: 48, 50: 49,   # 額外:骷髏
    51: 54,           # 額外:天使
}

def main():
    pack_path, t0_path, out = sys.argv[1], sys.argv[2], sys.argv[3]
    pack = Image.open(pack_path).convert('RGBA')
    t0 = Image.open(t0_path).convert('RGB')
    for slot, idx in SLOT_T0.items():
        src = t0.crop((0, idx*32, 32, idx*32+32)).resize((16, 16), Image.NEAREST).convert('RGBA')
        px = src.load()                       # 黑底→透明(sprite)
        for y in range(16):
            for x in range(16):
                r, g, b, a = px[x, y]
                if r < 24 and g < 24 and b < 24:
                    px[x, y] = (0, 0, 0, 0)
        # 先清掉該槽原內容(避免殘留),再貼
        pack.paste((0, 0, 0, 0), (slot*16, 0, slot*16+16, 16))
        pack.paste(src, (slot*16, 0), src)
    pack.save(out)
    print("wrote", out, "— 修正", len(SLOT_T0), "個 FM Towns 怪物槽")

if __name__ == '__main__':
    main()
