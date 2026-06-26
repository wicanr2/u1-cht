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

## 追加(2026-06-26):靜態解碼 5 種皆雜訊 + openMSX 不可裝
- 再試 4bpp linear 16×16 tile 網格 → 仍雜訊。累計 5 種佈局假設皆失敗。
- head `80 01 40 00 10 10 10 10 50 10 50 00 50 50 00 05 44 44...` 看似 outline+fill(像 tile),
  但無一佈局解出可辨圖 → 可能 tile/palette 經非顯然重排,或需動態 ground truth。
- **openMSX 不在 Ubuntu 24.04 apt**(需源碼編譯 + C-BIOS + 腳本跑到 overworld + VRAM dump + 找 tile)。
- ⇒ E3 屬 focused 多輪子任務:① 源碼編 openMSX 或 ② 找 MSX U1 反組譯/格式文件。

## 跨平台素材包整體結論(逐平台 RE 工程量)
- E1 FM Towns ✅ 完整(有 u7-cht 既抽 GRAPH + u2 解碼經驗才較快)。
- E2 IIgs / E3 MSX / E4 PC98 / E5 Atari:各需獨立格式 RE + palette 校準,**無現成工具**,
  每個都是 focused 子 session。基礎建設(E0 PNG AssetPack)已就緒,任何平台做出 832×16 PNG 即可載入。
