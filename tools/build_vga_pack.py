#!/usr/bin/env python3
"""組「VGA」tileset:用 EGA tileset 重新染色,把 16 色 EGA 擴成 256 色感的高彩版。

無原生 U1 VGA 素材(DOS 只有 EGA/CGA/Tandy)→ 以 EGA 為底:
  1. EGA-16 → 更自然飽和的 VGA 基色(色域擴大)。
  2. 每 tile 加柔和方向光 + ordered-dither 微紋理 → 平面色塊變多階調(用上幾百色 = VGA 質感)。
sprite(slot>=10)黑底→透明。用法:build_vga_pack.py <ega.png> <out.png>
"""
import sys
from PIL import Image

# EGA-16 RGB → VGA 重染基色(更自然/飽和)
VGA = {
    (0, 0, 0):       (0, 0, 0),
    (0, 0, 168):     (20, 36, 120),     # blue → 深海藍
    (0, 168, 0):     (46, 140, 46),     # green → 草綠
    (0, 168, 168):   (32, 150, 150),    # cyan
    (168, 0, 0):     (172, 36, 32),     # red
    (168, 0, 168):   (150, 44, 150),    # magenta
    (168, 84, 0):    (138, 88, 36),     # brown → 土棕
    (168, 168, 168): (150, 150, 160),   # ltgray → 石灰
    (84, 84, 84):    (78, 78, 88),      # dkgray
    (84, 84, 254):   (72, 112, 228),    # ltblue → 水藍
    (84, 254, 84):   (112, 216, 96),    # ltgreen
    (84, 254, 254):  (118, 226, 226),   # ltcyan
    (254, 84, 84):   (238, 92, 80),     # ltred
    (254, 84, 254):  (230, 104, 218),   # ltmagenta
    (254, 254, 84):  (244, 228, 96),    # yellow
    (254, 254, 254): (246, 246, 240),   # white
}
# 4×4 ordered-dither(Bayer)→ ±微擾,擴階調
BAYER = [[0,8,2,10],[12,4,14,6],[3,11,1,9],[15,7,13,5]]

def shade(rgb, x, y):
    # 方向光:左上亮、右下暗(±18%);+ Bayer 微紋理(±6)
    light = 1.0 + 0.16 * ((7.5 - y) / 7.5) + 0.10 * ((7.5 - x) / 7.5)
    d = (BAYER[y & 3][x & 3] - 7.5) * 0.8
    return tuple(max(0, min(255, int(c * light + d))) for c in rgb)

def main():
    ega_path, out = sys.argv[1], sys.argv[2]
    ega = Image.open(ega_path).convert('RGBA')
    n = ega.width // 16
    grid = Image.new('RGBA', (16*n, 16), (0, 0, 0, 0))
    for slot in range(n):
        tile = ega.crop((slot*16, 0, slot*16+16, 16))
        p = tile.load()
        out_t = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
        op = out_t.load()
        for y in range(16):
            for x in range(16):
                r, g, b, a = p[x, y]
                if (r, g, b) == (0, 0, 0):
                    # 地形保持黑(極暗);sprite 黑→透明
                    op[x, y] = (0, 0, 0, 0) if slot >= 10 else (0, 0, 0, 255)
                    continue
                base = VGA.get((r, g, b), (r, g, b))
                sr, sg, sb = shade(base, x, y)
                op[x, y] = (sr, sg, sb, 255)
        grid.paste(out_t, (slot*16, 0), out_t)
    grid.save(out)
    print("wrote", out, "— VGA 重染(EGA→豐富基色 + 光影 + dither)")

if __name__ == '__main__':
    main()
