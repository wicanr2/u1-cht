#!/usr/bin/env python3
"""從 Atari 8-bit《Ultima I》實機截圖切出 authentic overworld tile(16×16)。

依「遊戲實際圖比對」法(不跑模擬器):reference/hg101 的 atari800 overworld 截圖,
16×16 grid(自相關確認週期 16、offset (0,0) 對齊),切出 distinct tile。
目前單張 overworld 截圖(atari800-04)僅 4 種 tile 入鏡:water / grass / forest / player。

⚠ 完整 52 槽尚未達成,進度與下一步見 docs/re/atari-tileset-re.md。
用法:rip_atari_tiles.py <screenshot.png> <out_dir>
"""
import sys
from collections import Counter
from PIL import Image

TILE = 16

def main():
    shot, outdir = sys.argv[1], sys.argv[2]
    im = Image.open(shot).convert("RGB")
    W, H = im.size
    # 16×16 grid(offset 0,0),統計 distinct tile 頻率(避開底部文字區)
    freq = Counter()
    pos = {}
    for cy in range(0, min(H - TILE, 160), TILE):
        for cx in range(0, W - TILE, TILE):
            key = im.crop((cx, cy, cx + TILE, cy + TILE)).tobytes()
            freq[key] += 1
            pos.setdefault(key, (cx, cy))
    # 高頻 = 地形(grass/forest/water),罕見 = player
    names = ["grass", "forest", "water", "player"]
    import os
    os.makedirs(outdir, exist_ok=True)
    for i, (key, n) in enumerate(freq.most_common(len(names))):
        cx, cy = pos[key]
        t = im.crop((cx, cy, cx + TILE, cy + TILE))
        t.save(os.path.join(outdir, f"atari_{names[i]}.png"))
        print(f"{names[i]:8s} @({cx},{cy}) freq={n}")

if __name__ == "__main__":
    main()
