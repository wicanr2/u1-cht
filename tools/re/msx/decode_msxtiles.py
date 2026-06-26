#!/usr/bin/env python3
"""
decode_msxtiles.py — 解碼 MSX 版 Ultima I 的 tile 圖檔(MSXTILES.BIN / MSXTOWN.BIN / MSXDANG.BIN）

格式由反組譯 OUT.COM(Z80)得出,見 docs/re/e3-msx.md 的「MSXTILES 格式(Path B 反組譯結論)」。

關鍵結論(OUT.COM sub_8910h / sub_8999h):
  * 檔頭 4 bytes:word1(LE)、word2(LE)。word1 / 2 = 每頁寫入 byte 數(MSXTILES=0x180/2=192);
    word2 = 頁數(MSXTILES=0x40=64)。body = word2 頁 × (word1/2) byte = 12288。
  * loader 把 body 線性寫進 VRAM,但每寫 (word1/2) bytes 就 `inc h`(VRAM dest += 0x100,跳下一個 256-byte page）。
    即 body offset (p*N + i) → VRAM (p*256 + i),N=word1/2,p=0..word2-1,i=0..N-1;每頁尾 (256-N) bytes 留空。
  * 顯示模式 = SCREEN 7(GRAPHIC 6,512 寬,stride 256 bytes/line,chunky 4bpp,高 nibble = 左像素)。
    ⇒ 每個 page(256 VRAM bytes)= 一條掃描線;192 bytes = 該線前 384 px。
  * MSXTILES 共 64 線 × 384 px = 24 欄 × 4 列 的 16×16 tiles = 96 tiles(MSX 原生順序)。
  * palette(openMSX VRAM dump 實測,V9938 0RRR0BBB / 00000GGG):8 色,index = G<<2 | R<<1 | B
      0黑 1藍 2紅 3洋紅 4綠 5青 6黃 7白  ← 先前文件 1/4 寫反,此為正解。

用法:
  python3 decode_msxtiles.py MSXTILES.BIN out.png [--tile-cols 24]
產出:raw MSX 順序的 tile sheet PNG(每 tile 16×16,黑底)。tile→engine slot 對映另見 build_msx_pack.py。
"""
import sys, struct
from PIL import Image

# V9938 8 色 GRB palette(index = G<<2 | R<<1 | B),3-bit→8-bit 只用 0/7 → 0/255
PAL = [(0,0,0),(0,0,255),(255,0,0),(255,0,255),(0,255,0),(0,255,255),(255,255,0),(255,255,255)] + [(0,0,0)]*8

def load_body(path):
    raw = open(path, 'rb').read()
    w1, w2 = struct.unpack('<HH', raw[:4])
    body = raw[4:]
    N = w1 // 2                  # bytes/line(實際寫入)
    pages = w2                   # 線數
    px_per_line = N * 2          # 每線像素寬
    assert len(body) == N * pages, f"body {len(body)} != {N}*{pages}"
    return body, N, pages, px_per_line

def render(body, N, pages, px_per_line, out, tile=16):
    W, H = px_per_line, pages
    img = Image.new('RGB', (W, H))
    p = img.load()
    for y in range(H):
        for x in range(W):
            b = body[y*N + (x >> 1)]
            n = (b >> 4) if (x & 1) == 0 else (b & 0xF)
            p[x, y] = PAL[n & 15]
    img.save(out)
    cols = W // tile; rows = H // tile
    print(f"{out}: {W}x{H}  = {cols}x{rows} grid of {tile}x{tile} = {cols*rows} tiles")
    return img

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(__doc__); sys.exit(1)
    body, N, pages, ppl = load_body(sys.argv[1])
    print(f"header: bytes/line={N} lines={pages} width={ppl}px")
    render(body, N, pages, ppl, sys.argv[2])
