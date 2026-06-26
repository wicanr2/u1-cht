# 6502 老遊戲反組譯判讀經驗(Ultima I Atari 實戰)

> 從 1983 Atari Ultima I 反追怪物 AI 過程中累積的可複用方法與踩雷。
> 對象:無符號、有拷貝保護、overlay 結構的 8-bit 老遊戲。配 `rulebook/62-static-provenance-trace`。

## 0. 總流程(image → 行為)

```
盤點映像 → 辨識格式/版本 → 抽檔系統 → 解 binary-load 分段 →
分離 code/data(遞迴下降)→ 認 ZP 變數/資料表 → 錨定 sink → 反追演算法
```
每步只記**已驗證事實**,推測標「待驗」。別跳步、別憑檔名/外觀採信。

## 1. 先驗版本,別被檔名騙(踩雷)

`Ultima I IIgs.woz` 檔名像「原版 Apple」,解 WOZ2 的 **META chunk** 才發現是
**1994 Rebecca Heineman / Vitesse 的 IIgs 移植**(`copyright 1994`、`requires_machine 2gs`)——
差原版 13 年、不同作者。**教訓:斷言「這是原版」前先讀 metadata 驗證**(對齊 `62` 的「描述前先驗證」)。

- WOZ2 結構:header(12B)+ chunks `INFO`/`TMAP`/`TRKS`/`META`。`INFO.disk_type`(1=5.25,2=3.5)、
  `INFO.write_protected`;`META` 有 `title/developer/publisher/copyright/requires_machine`。
- 4am「woz-a-day」保存原始拷貝保護 → 標準檔系統工具(AppleCommander)常**讀不到 catalog**(回 null);
  保護片要先破/轉,版本不對就別在這浪費時間。

## 2. 選最可讀的 oracle

同遊戲多平台時,可讀性差很多:
- **Apple woz**:nibble 級 + 常有拷貝保護 → 難。
- **Atari ATR**:標準 sector + 標準 Atari DOS 2 檔系統 → **好讀**(本案勝出)。
- **編譯 EXE / ISO**:邏輯被編譯掉 → 當行為對照可,當「原始邏輯 oracle」差。
- **原版若是 BASIC**(如 1981 Apple U1 Applesoft)→ detokenize 後最好讀;可惜本案手上最早是 Atari(asm)。

## 3. Atari 抽檔:DOS 2 sector chain + binary load 格式

- **ATR header**:8 bytes,magic `96 02`(0x0296);之後是 sector 資料(單密度 128B/sector)。
- **Atari DOS 2 目錄**:sector 361–368,每項 16B:`flag(1) count(2 LE) start(2 LE) name(8) ext(3)`,
  `flag & 0x40` = 使用中。
- **檔案 sector chain**:每 128B sector 末 3 byte = `[file#<<2 | next_hi2][next_lo8][valid_byte_count]`;
  next=0 結束。工具:`tools/re/atari_extract.py`。
- **Atari binary load(可執行檔)**:`FF FF` 開頭,後接多個 `(start_LE, end_LE, data)` 分段;
  `$02E0`=RUNAD(執行位址)、`$02E2`=INITAD(載入時初始化)。重建 64K 映像後再反組譯。
- **大彩蛋:檔名自帶語意** —— U1 Atari 把碼**按功能分檔**:`OUTMOVE`(地面移動)、`DNGMOVE`(地牢移動)、
  `TWNMOVE`/`CASMOVE`/`SPAMOVE`、`DNGOBJ`。**先用檔名鎖定要看的模組**,省大把時間。

## 4. 反組譯:linear sweep 會騙你,要遞迴下降

- **[雷] `da65` linear sweep 把資料當碼**:本案 1395–1495 行一堆 `inc $88,x` / `asl $2F50` / `brk`,
  看似「怪物座標被遞增」的 AI——其實是**資料表 byte 巧合解成合法 opcode**。差點誤判。
- **正解:遞迴下降反組譯**(`tools/re/dis6502.py`):從進入點跟 `JSR/JMP/分支/fall-through` 走,
  只把**真正到達**的 byte 當 code,其餘留 data。非法 opcode / `RTS/RTI/BRK` / `JMP` 即停。
- **判斷 code vs data 的快訊號**:`reached / total` 覆蓋率。本案從 $8000 只到 48–51 bytes →
  **$8000+ 那 9.9KB 大半是資料**(地牢 3D 線框頂點 + `$0F` graphics mask + 位元遮罩表),
  程式碼其實很小。`$8000: jmp $80B3`(跳過中間資料表)是「code 夾 data」的典型。

## 5. 找進入點(overlay 最麻煩的一步)

- overlay 模組的 routine 由**主程式從固定位址 JSR**;模組自己 `RUNAD` 可能是 0(非獨立執行)。
- **掃 `$20 lo hi`(JSR opcode)目標**當候選入口種子,全餵進遞迴下降提覆蓋。本案全模組只有 4 個內部
  JSR 目標 → 再次印證「碼少、資料多、調度在外」。
- **外部位址 = 跨模組線索**:DNGMOVE 的碼一直跳到 `$0269` / `$30xx`(不在本檔內)→ 表示
  **地牢回合的調度在 ULTIMA 主程式($30xx 區)**,DNGMOVE 只提供 helper。`$6C45` 把 `$6A75` 寫進
  `$3019`(patch 主程式的向量)= 模組「向主程式註冊自己的 callback」。**追完整邏輯必須跨模組**。

## 6. 認資料結構與常見 6502 慣用法

- **單位元遮罩表** `80 40 20 10 08 04 02 01`(@$6C6E)+ `操作數 & 7` → bit index → 查表取 mask:
  典型 **1-bit/cell 點陣圖**存取(本案地牢牆是位元圖)。
- **座標→位址計算**:連續 `ASL/ROL`(×2 累乘做 row×width)+ `ADC #lo / ADC #hi` 拼出 base(如 `$A6F0`)
  + `ORA ($ptr),y` 寫位元 → 「給 (x,y) 算出地圖 byte 位址並設/取一個 bit」。
- **ZP 索引 = 陣列**:`lda/inc $NN,x`(zero-page,X)幾乎都是「以索引存取的小陣列」(怪物表、座標表候選)——
  但**先確認那段是 code 不是 data**(見第 4 點)再採信。
- **16-bit 指標在 ZP**:`$C0/$C1`、`$C2/...` 成對,搭 `(zp),y` 間接定址 = C 的 `ptr[y]`。

## 7. 反追演算法的錨點策略(sink-first)

字串錨點不一定有(本案 DNGMOVE 只有 DOS 訊息與狀態列字 `H.P.= FOOD=`,**無怪名/移動訊息**)。
退而求其次的錨點:
- **共用 helper 的 caller**:地圖位元圖存取器($6A00/$6C28)被誰呼叫 → 那些 caller 多是「移動/碰撞」邏輯。
- **玩家座標 ZP 變數**:找「玩家 dungeon X/Y」的讀寫點,其鄰近 `CMP 怪座標 → 分支 → INC/DEC` 即移動 AI。
- **跨模組**:本案需先抽 `ULTIMA`/`MASTER`,在 $30xx 找地牢回合 handler,再回看它對 DNGMOVE 的呼叫。

## 8. 工具清單(本 repo)

| 工具 | 用途 |
|---|---|
| `tools/re/atari_extract.py` | ATR → Atari DOS 2 抽檔(sector chain) |
| `tools/re/dis6502.py` | 遞迴下降 6502 反組譯(分離 code/data + JSR 呼叫圖) |
| `docker/Dockerfile.re` | wozardry / AppleCommander / cc65(da65)工具鏈 |

## 9. 一句話心法

> **覆蓋率低不是「反組譯失敗」,是「這塊是資料」的證據;碼一直往外跳不是「斷了」,是「調度在別的模組」。**
> 老遊戲的「move 模組」常是一堆渲染資料 + 幾個 helper,真正的回合邏輯在主程式——**順著外部位址跨模組追**。
