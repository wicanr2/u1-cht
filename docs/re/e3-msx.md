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

## ★★ openMSX 動態 RE 完整發現(2026-06-26)
openMSX 在 docker 編成並跑起遊戲(見 `openmsx-build-experience.md`),挖出:
1. **VDP 模式 = SCREEN 7;palette = 8 色 GRB**(`0黑1綠2紅3黃4藍5青6洋紅7白`,index=B<<2|R<<1|G)。
   = MSXTILES 解碼**正確 palette**(先前靜態全猜錯)。
2. **遊戲流程**:`OUT` → 片頭 credits → 主選單(a建角/b開始)→ 建角(上下選/左右增減/SPACE完成,30點分散到6項,單項有上限)。
3. **★ 雙磁碟存檔系統**:建角完成後要求「キャラクターディスクを ドライブ1にいれてください」
   = 需**第二張空白角色碟**才能存角色 + 進 overworld(`docs/re/img/msx_chardisk_prompt.png`)。
   單磁碟機 → 需 disk-swap(程式碟↔角色碟)。已建空白碟 `CHARDISK.dsk`(合法 FAT12)。
4. **MSXTILES 在 overworld 才載入 VRAM**(片頭/選單/建角的 VRAM 都無 MSXTILES 內容)。

## 卡點 + 兩條續作路徑
**卡點**:**絕對時間腳本無法可靠導航**——片頭長度每次飄移,加上多步建角 + 雙碟 swap,
動作落點不準,反覆 reset 回建角/選單。需**事件驅動**(偵測畫面/VRAM signature 再動作)。

| 路徑 | 作法 | 評估 |
|---|---|---|
| **A. 事件驅動 emulator 導航** | 寫 Tcl 偵測當前畫面(VRAM 特徵/OCR)→ 狀態機跑完建角+雙碟swap→overworld dump | 可靠但要寫狀態機 |
| **B. 反組譯 `OUT.COM`(Z80)** | OUT.COM 是 MSXTILES→VRAM 的 loader;Z80 反組譯找它的 VRAM 寫入格式/位址 | 純靜態,連回 6502 經驗;不必導航遊戲 |

→ **建議 B**(靜態反追 loader,避開不可靠的遊戲導航;且已有 6502 方法論可借鏡到 Z80)。
palette 已知(8色GRB)、mode 已知(SCREEN7),只差「MSXTILES 檔 → VRAM 的搬運/轉換格式」,
OUT.COM 反組譯應能直接給出。
