# E3 MSX 素材抽取 — 進展(2026-06-26)

## ✅ 已通
- 素材:`org_game/msx/…(Pony Canyon)(ja).zip` → `.dsk`(720KB,標準 **FAT12 MSX-DOS**,OEM "CANONFDD")。
- FAT12 reader 抽出檔案:**`MSXTILES.BIN`(12292)= overworld tileset**、`MSXTOWN`/`MSXDANG`(地牢)/
  `FONT.BIN`/`MAP.BIN`/`TCD.BIN`,以及 `OUT/TOWN/DG/SPACE/MONDAIN.COM`(各情境程式,同 Atari 模組化)。
- MSXTILES 結構化非壓縮(entropy 4.54;最常見 byte 0x00/0x88/0x77/0x44 = 同 nibble 純色)。

## 🔄 format 未破(tile 解碼)
12292 = 4 byte header + 12288。試過皆雜訊:
- SCREEN2 pattern[6144]+color[6144](兩種順序)
- 4bpp linear(SCREEN5 style,16×16 tile / 128寬 strip)
- per-char 交錯(8 pattern + 8 color × 768)

→ 真實佈局非顯然。**建議**:用 **openMSX 模擬器**載入跑到 overworld,從 VRAM dump 比對
MSXTILES 的實際 pattern/color/palette 佈局(同 FM Towns 需對實機校 palette);或找 MSX U1 反組譯。

## 與其他平台一致的觀察
每個非 DOS 平台(MSX/PC98/IIgs)都是**獨立硬體格式 + 可程式 palette**,需逐版 RE + 實機/模擬器校準。
E1 FM Towns 是已完成的完整範例(`docs/re/fmtowns-u1-graphics.md`),流程可參考但格式各異。
