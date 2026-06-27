#!/usr/bin/env python3
"""產生 README 的多平台 tileset 對比圖(docs/img/tilesets_compare.png)。

來源 = 各平台 in-game overworld 截圖(docs/img/screen_<platform>.png,由 game_tester 切 tileset 後截)。
每格裁同一塊 overworld 區域 + 平台標籤,排成網格。新增平台只要補一張 screen_<x>.png 並加進 PANELS。
"""
from PIL import Image, ImageDraw, ImageFont

PANELS = [
    ("EGA", "screen_ega.png"),
    ("CGA", "screen_cga.png"),
    ("FM Towns", "screen_fmtowns.png"),
    ("MSX", "screen_msx.png"),
    ("PC-98", "screen_pc98.png"),
    ("Apple IIgs", "screen_iigs.png"),
    ("Atari 8-bit", "screen_atari.png"),
]
IMGDIR = "docs/img/"
CROP = (40, 36, 40+576, 36+360)   # overworld 取景(避開狀態列/邊框)
PW, PH = 384, 240                 # 每格縮放後尺寸
LABEL_H = 24
COLS = 2

def font():
    for p in ("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"):
        try: return ImageFont.truetype(p, 16)
        except Exception: pass
    return ImageFont.load_default()

def main():
    f = font()
    rows = (len(PANELS) + COLS - 1) // COLS
    pad = 6
    cellw, cellh = PW + pad, PH + LABEL_H + pad
    canvas = Image.new("RGB", (cellw*COLS + pad, cellh*rows + pad), (24, 24, 28))
    d = ImageDraw.Draw(canvas)
    for i, (name, fn) in enumerate(PANELS):
        im = Image.open(IMGDIR + fn).convert("RGB").crop(CROP).resize((PW, PH), Image.NEAREST)
        cx = pad + (i % COLS) * cellw
        cy = pad + (i // COLS) * cellh
        d.rectangle([cx, cy, cx+PW-1, cy+LABEL_H-1], fill=(40, 40, 48))
        d.text((cx+8, cy+3), name, font=f, fill=(255, 220, 120))
        canvas.paste(im, (cx, cy+LABEL_H))
    canvas.save(IMGDIR + "tilesets_compare.png")
    print("wrote", IMGDIR + "tilesets_compare.png", canvas.size)

if __name__ == "__main__":
    main()
