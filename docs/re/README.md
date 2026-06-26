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

## 下一步(計畫,未執行)

- **Step 3**:抽 Apple woz 檔系統 → 列檔 → 確認 BASIC/binary 組成。
  工具鏈:`wozardry`(woz→dsk)+ `AppleCommander`(DOS 3.3 讀檔 / Applesoft detokenize)。
- **Step 4**:若有 BASIC 主程式 → detokenize → 定位怪物移動/攻擊段落(grep `MONSTER`/座標運算)。
- **Step 5**:逐句反追怪物移動規則(每回合幾格?貪婪逐玩家?對角?亂數?),內容對齊驗證。
- **Step 6**:把驗證後的規則寫進 `docs/ultima1-original-ai.md`,再據以實作 open_ultima 地牢移動。

## 待驗 / 未決(不下結論)

- woz 是 1981 原版還是 1986 Origin 重製?(影響 AI 是否與後續版本一致)— 抽檔看版本字串再定。
- Apple 版怪物 AI 是在 BASIC 還是 assembly 子程式?— 抽檔後確認。
- 「地牢怪 beeline + 對角攻擊」目前僅網路二手(見 `ultima1-original-ai.md`),**尚未由原碼證實**。
