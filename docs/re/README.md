# Ultima I 原版逆向分析(怪物 AI oracle)

> 目的:從**原版 U1 磁碟映像**靜態反追怪物 AI 行為,當 open_ultima 實作的權威依據。
> 紀律:`rulebook/62-static-provenance-trace`(先靜態反追、邊驗證邊記錄、**不急著下結論**)。
> 進行中,逐步更新。每步只記**已驗證的事實**,推測標「待驗」。

## Step 1 — 映像盤點(2026-06-26)

| 映像(`org_game/`) | 版本 | 格式 | oracle 評估 |
|---|---|---|---|
| `Ultima I IIgs.woz` | Apple(4am woz-a-day 保存) | WOZ2(nibble 級) | **最接近原版**;woz 最難抽 |
| `Ultima I …Atari…Side A/B.atr/.atx` | 1983 SierraVenture(Atari 8-bit) | ATR(標準 sector) | 原版年代;但見 Step 2 = 6502 機器碼 |
| `disk1.img`(IBM PC) | 編譯 EXE | FAT12 | ❌ 編譯檔,非邏輯 oracle |
| `Ultima_Trilogy_1989.iso` | FM Towns/DOS 編譯 | ISO | ❌ 編譯檔 |

## Step 2 — 格式辨識(已驗證)

- **Atari ATR**:檔頭 `96 02`(ATR magic)、40 軌 18 磁區。第一磁區 dump 開頭含 `4C`(6502 `JMP`)
  → **Atari 版是 6502 機器碼**(assembly),非 Atari BASIC。⇒ 要反組譯,讀起來較硬。
- **Apple WOZ**:檔頭 `WOZ2`(WOZ 2.0,Apple II nibble 映像,4am 保存含原始拷貝保護)。
  原版 Apple U1(1981/1986)歷史上以 **Applesoft BASIC + 少量 assembly** 寫成 →
  若 BASIC 在,detokenize 後**最好讀**,是首選 oracle。**(待 Step 3 抽檔驗證)**

## Step 3 — woz 身分鑑定(重要更正,已驗證)

解析 `Ultima I IIgs.woz` 的 INFO/META chunk:
- `disk_type=2`(**3.5"**,非 5.25")、`requires_machine: 2gs`、`requires_rom: IIgs ROM1+3`、`write_protected=0`。
- META:`developer: Richard Garriott | **Rebecca Heineman** | Scott Everts…`、`publisher: **Vitesse**`、
  `copyright: **1994**`、`title: Ultima I / The First Age of Darkness`。

⇒ **這片是 1994 年 Rebecca Heineman / Vitesse 的 Apple IIgs 移植版**(65816 組語),**不是 1981 原版**。
當「原版怪物 AI oracle」不理想(差 13 年、不同作者與架構)。**教訓**:差點因檔名「IIgs / woz-a-day」
就誤當原版 Apple——META 一驗就翻案(對齊 `62` 鐵則:描述/採信前先驗證)。

> 註:此 IIgs 片 `write_protected=0`、3.5" ProDOS,理論上可讀,但版本不對,**降優先**。
> AppleCommander `-l` 回 null(可能需 woz→2mg;因版本不對暫不深追)。

## 候選 oracle 重排(Step 3 後)

| 映像 | 年代/作者 | 與 1981 原版距離 | 可讀性 | 結論 |
|---|---|---|---|---|
| Atari `.atr`(SierraVenture) | **1983** | **最近**(早期港) | 標準 ATR sector,可讀 | ✅ **改用這個當主 oracle** |
| Apple `Ultima I IIgs.woz` | 1994 Heineman/Vitesse | 遠(13 年後重寫) | 3.5" ProDOS(woz) | ⏸ 版本不對,備用 |
| IBM PC / Trilogy | 1987+ 編譯 | 中 | 編譯檔 | ❌ |

> 1981 Apple 原版**不在** `org_game/`;手上最早是 1983 Atari。以它為主 oracle(6502 asm 反組譯)。

## 下一步(計畫,未執行)

- **Step 3**:抽 Apple woz 檔系統 → 列檔 → 確認 BASIC/binary 組成。
  工具鏈:`wozardry`(woz→dsk)+ `AppleCommander`(DOS 3.3 讀檔 / Applesoft detokenize)。
- **Step 4**:若有 BASIC 主程式 → detokenize → 定位怪物移動/攻擊段落(grep `MONSTER`/座標運算)。
- **Step 5**:逐句反追怪物移動規則(每回合幾格?貪婪逐玩家?對角?亂數?),內容對齊驗證。
- **Step 6**:把驗證後的規則寫進 `docs/ultima1-original-ai.md`,再據以實作 open_ultima 地牢移動。

## Step 4 — Atari 檔系統(大突破,已驗證)

Atari Side A(Program Master)是**標準 Atari DOS 2 檔系統**(非自訂 boot),且**程式碼按功能模組分檔、檔名自帶語意**:

| 檔案 | 推測 | sectors |
|---|---|---|
| `OUTMOVE` | 地面(outworld)移動 | 75 |
| **`DNGMOVE`** | **地牢移動(怪物 AI 應在此)** | 86 |
| `TWNMOVE` / `CASMOVE` / `SPAMOVE` | 城鎮/城堡/太空移動 | — |
| `DNGOBJ` | 地牢物件/資料 | 10 |
| `ULTIMA` / `INITDSP` / `INITLOAD` / `MASTER` | 主程式 / 初始化 | — |
| `SET1`–`SET5` | 資料集 | — |

提取工具:`tools/re/atari_extract.py`(Atari DOS 2 sector chain + binary-load 分段解析,可重跑)。
抽出檔皆 **Atari binary load 格式**(`FFFF` 標頭)。

## Step 5 — DNGMOVE 反組譯(進行中)

- 分段:bootstrap `$6A00-$6C75` + 主碼 **`$8000-$A68E`(~9.9KB)** + RUNAD 向量。
- `da65 --start-addr 0x8000` → **5937 行 6502**(`re_work/atari/DNGMOVE_main.s`)。
- **錨點嘗試**:抽 DNGMOVE/DNGOBJ 字串,多為 Atari DOS 訊息(`ERROR/FILE/WRITE PROTECT`)
  與狀態列(`H.P.= FOOD= EXP.= GOLD=`、`INVENTORY/PLAYER/LEVEL`、寶石名);**無直接怪名/移動訊息**可錨。
- ⇒ 真正錨點在 **zero-page 的玩家/怪座標變數 + 地牢資料結構**;需在 5937 行內逐段認出
  「比對怪座標 vs 玩家座標 → INC/DEC 怪座標」的子程式。**深度互動式 6502 追蹤,多輪工程。**

### Step 6 — 結構解明(遞迴下降後,已驗證)

用自製遞迴下降反組譯器(`tools/re/dis6502.py`)分離 code/data,翻案 da65 linear sweep 的誤判:

- **da65 linear 的 `inc $88,x` 是假指令**:1395–1495 行是**資料表**被當碼解(`62` 紀律:描述前先驗證)。
- **$8000–$A68E(9.9KB)大半是資料**:`$8000: jmp $80B3` 跳過中間 `$0F` graphics mask /
  3D 線框頂點表;從 $8000/$80B3 遞迴只到達 ~50 bytes → 碼很少。
- **DNGMOVE 是 overlay,真正調度在 ULTIMA 主程式**:碼一直跳外部 `$0269`/`$30xx`;
  `$6C45` 把 `$6A75` 寫進 `$3019`(向主程式註冊 callback)。全模組僅 4 個內部 JSR 目標。
- **已認出的 helper**:`$6A00`+`$6C28` = **地牢地圖位元圖存取器**(座標→位址+bit;
  遮罩表 `$6C6E = 80 40 20 10 08 04 02 01`,1-bit/cell 表牆)。

### 狀態:DNGMOVE 解明為「資料 + helper」,AI 調度在 ULTIMA(跨模組)
> **未受阻**,下一步明確:**抽 `ULTIMA`/`MASTER` → 在 $30xx 找地牢回合 handler →
> 看它每回合對怪座標的處理**。怪物移動 AI 在主程式的回合迴圈,不在 DNGMOVE 本身。
> 6502 判讀經驗已整理:見 `docs/re/6502-re-methodology.md`。

## 待驗 / 未決(不下結論)

- woz 是 1981 原版還是 1986 Origin 重製?(影響 AI 是否與後續版本一致)— 抽檔看版本字串再定。
- Apple 版怪物 AI 是在 BASIC 還是 assembly 子程式?— 抽檔後確認。
- 「地牢怪 beeline + 對角攻擊」目前僅網路二手(見 `ultima1-original-ai.md`),**尚未由原碼證實**。
