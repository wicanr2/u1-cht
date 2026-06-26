#!/usr/bin/env python3
"""組 Apple IIgs U1 PNG AssetPack(832×16,對齊 engine 52 槽)。

來源:hg101 實機 **320×200 原生 SHR 截圖**(`reference/hg101/imgs/ultima1-appleIIgs-{07,12}.png`),
直接切 16×16 tile(overworld 網格 offset (8,8))。見 docs/re/e2-apple-iigs.md。
取得 8 個真實 IIgs overworld tile:水/草/森林/山/城堡/城鎮/玩家/馬。
其餘槽(signpost/dungeon/載具/怪物)IIgs 截圖未涵蓋 → 以 EGA tileset 補(hybrid,可玩)。

player/horse 截圖含草地背景 → 去綠(grass)轉透明,當 sprite 疊在地形上。
用法:build_iigs_pack.py <ega.png> <out.png>
"""
import sys, hashlib
from PIL import Image

OFF = 8  # overworld tile 網格相位(玩家居中推得)

def uniq_tiles(path, x0=8, y0=8, x1=312, y1=152):
    """回傳 dedup 後 unique tiles,依出現次數遞減排序。"""
    im = Image.open(path).convert('RGB'); out = {}
    for cy in range(y0, y1, 16):
        for cx in range(x0, x1, 16):
            if cx+16 > 320 or cy+16 > 200: continue
            t = im.crop((cx, cy, cx+16, cy+16))
            h = hashlib.md5(t.tobytes()).hexdigest()
            if h not in out: out[h] = [t, 0]
            out[h][1] += 1
    return [v[0] for v in sorted(out.values(), key=lambda v: -v[1])]

def grass_to_alpha(tile):
    """把草地(綠)像素轉透明,留下 sprite(player/horse)。"""
    t = tile.convert('RGBA'); px = t.load()
    for y in range(16):
        for x in range(16):
            r, g, b, a = px[x, y]
            if g > 60 and g >= r and g > b:  # 偏綠 = 草地背景
                px[x, y] = (0, 0, 0, 0)
    return t

def main():
    ega_path, out = sys.argv[1], sys.argv[2]
    s07 = uniq_tiles('reference/hg101/imgs/ultima1-appleIIgs-07.png')  # 0草1森2水3堡4玩家
    s12 = uniq_tiles('reference/hg101/imgs/ultima1-appleIIgs-12.png')  # 0草1森2水3山4鎮5馬
    iigs = {                       # engine slot → IIgs tile(RGB,terrain 用)
        0: s07[2],   # WATER
        1: s07[0],   # GRASS
        2: s07[1],   # FOREST
        3: s12[3],   # MOUNTAIN
        4: s07[3],   # CASTLE
        5: s07[3],   # CASTLE swap
        7: s12[4],   # TOWN
        8: s12[4],   # TOWN swap
    }
    sprites = {                    # slot → IIgs sprite(去草地透明)
        10: grass_to_alpha(s07[4]),  # PLAYER
        11: grass_to_alpha(s12[5]),  # HORSE
    }
    ega = Image.open(ega_path).convert('RGBA')
    grid = Image.new('RGBA', (16*52, 16), (0, 0, 0, 0))
    for slot in range(52):
        if slot in iigs:
            grid.paste(iigs[slot].convert('RGBA'), (slot*16, 0))
        elif slot in sprites:
            t = sprites[slot]; grid.paste(t, (slot*16, 0), t)
        else:
            # 沿用 EGA;slot>=10 黑底→透明
            t = ega.crop((slot*16, 0, slot*16+16, 16))
            if slot >= 10:
                px = t.load()
                for y in range(16):
                    for x in range(16):
                        r, g, b, a = px[x, y]
                        if r == 0 and g == 0 and b == 0: px[x, y] = (0, 0, 0, 0)
            grid.paste(t, (slot*16, 0), t)
    grid.save(out)
    print("wrote", out, "— 8 real IIgs tiles (water/grass/forest/mountain/castle/town/player/horse) + EGA fallback")

if __name__ == '__main__':
    main()
