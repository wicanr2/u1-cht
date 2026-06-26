# E3 MSX 素材抽取 — 進展(2026-06-26)

## ★★★ Path B 成功:MSXTILES 格式完全破解(2026-06-26)

反組譯 `OUT.COM`(Z80,`u1-z80` docker 內 z80dasm,origin 0x100)直接給出 file→VRAM 轉換,**不必導航遊戲**。

### loader 結構(OUT.COM)
- `0x3c00` 區段依序載 4 檔:`sub_8910h(DE=VRAM dest, HL=檔名字串)`
  → MSXTILES.BIN(dest 0x0000)、SKULL.BIN(0xc0)、MSXTOWN.BIN(0x4000)、MSXDANG.BIN(0x5000)。
- `sub_8910h`:讀 4-byte 檔頭 word1/word2;`N = word1/2`(每線寫入 byte),`pages = word2`(線數)。
  雙層迴圈:每寫 N bytes 就 `inc h`(VRAM dest += 0x100,跳下一個 256-byte page)。
- `sub_8999h`:V9938 VRAM 寫(R14 設 A14-16:`(H&0xC0)>>6`;port 0x99 設位址 + 0x98 寫 data)。

### 轉換公式(關鍵)
```
body offset (p*N + i)  →  VRAM (p*256 + i)     p=0..pages-1, i=0..N-1
```
每頁尾 (256-N) bytes 留空。顯示模式 **SCREEN 7(GRAPHIC 6,512 寬,stride 256 B/line,chunky 4bpp,高 nibble=左)**
⇒ 每 page = 1 條掃描線,N=192 bytes = 該線前 384px。

### 結果
- MSXTILES.BIN(64 線)= **384×64 = 24×4 格 16×16 = 96 tiles**(MSX 原生順序,清晰可辨:水/草/森林/城堡/城鎮/船/山/龍蛇/騎士/骷髏/惡魔/NPC/特效)。
- MSXTOWN.BIN(16 線)= 24 tiles(城鎮文字字型,解出可讀「JKLM…XYZ 0123…F」)。
- MSXDANG.BIN(128 線)= 192 tiles(地牢)。
- 工具:`tools/re/msx/decode_msxtiles.py`(通用,吃任一 MSX*.BIN → tile sheet PNG)。

### ⚠ palette 修正(實測 openMSX VDP palette dump)
V9938 palette 格式 `0RRR0BBB`/`00000GGG`,實測 8 色 = **index = G<<2 | R<<1 | B**:
`0黑 1藍 2紅 3洋紅 4綠 5青 6黃 7白`。**先前文件寫「1綠4藍 / index=B<<2|R<<1|G」是反的**(把藍綠對調)。
以本條為準。`44`=綠=草、`11`=藍=水。

> 卡點(下方 Path A 的導航時序問題)已不影響:Path B 靜態反組譯繞過了「跑到 overworld」的需求。
> 借用 `docs/re/6502-re-methodology.md` 方法論(換 Z80 / z80dasm)即達標。

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
