# Apple IIgs 版 Ultima I(ULTIMAI)65816 反組譯 — 第一性原理逐步紀錄

> 目標:反組譯 1994 Vitesse/Heineman 版 ULTIMAI(65816),找出 **type 0x0001 resource 的自訂解壓常式**,
> 以解出 overworld 載具/怪物 tile(截圖 Rosetta / MAME 滑鼠 / 未壓縮搜尋皆受阻,見 `e2-apple-iigs.md`)。
> 方法論借 `6502-re-methodology.md`(遞迴下降反組譯、別把 data 當 code)+ 靜態溯源紀律(`rulebook/62`)。
> **每一步都記在這裡**,可重跑、可回溯。

材料:`re_work/iigs/ULTIMAI`(data fork,90621 bytes;從 woz 經 floptool 抽出,見 `e2-apple-iigs.md`)。

---

## Step 1 — 檔案格式:GS/OS OMF 可執行檔(ExpressLoad)

第一性問題:這 90621 bytes 是什麼?GS/OS 應用程式是 **OMF**(Object Module Format)—— 一串「段(segment)」,
每段一個 header + 可重定位的 body(LCONST 常數 + RELOC 重定位記錄)。寫了 OMF v2 header 解析器(`tools/re/iigs/omf_parse.py`)。

### OMF v2 segment header(關鍵欄位,LE)
| offset | 欄位 | 意義 |
|---|---|---|
| 0 | BYTECNT(4) | 本段在檔案佔的 bytes(下一段 = off + BYTECNT) |
| 8 | LENGTH(4) | 載入到記憶體的長度 |
| 14 | NUMLEN(1)=4 / VERSION(1)=2 | OMF v2 確認 |
| 16 | BANKSIZE(4)=0x10000 | 64KB bank |
| 20 | KIND(2) | 0x0000=CODE、0x0001=DATA、bit15=dynamic |
| 36 | ENTRY(4) | 進入點 |
| 42 | DISPDATA(2) | body(OMF record 串)在段內的起始 offset |

### ULTIMAI 段表(解析結果)
| seg# | 檔案 offset | BYTECNT | mem LENGTH | KIND | 名稱 | 內容 |
|---|---|---|---|---|---|---|
| 1 | 0x00000 | 235 | 162 | 0x8001 DATA(dyn) | (含 `ExpressLoad` 字串) | **ExpressLoad** 目錄段(加速載入用) |
| 2 | 0x000eb | 61004 | 52722 | 0x0000 CODE | `UltimaI` | **★ 主遊戲程式碼**(解壓常式在這) |
| 3 | 0x0ef37 | 29382 | 29312 | 0x0000 CODE | `UltimaData` | 資料/次要碼段 |

**第一性結論**:
- ULTIMAI 是 **ExpressLoad** 過的 GS/OS app(seg#1 是 ExpressLoad 目錄;這是 Apple 的預重定位格式,載入快)。
- 主程式碼 = **seg#2**(52722 bytes 65816 機器碼),body 從段內 offset DISPDATA 起,是 OMF record 串
  (LCONST=0xF2 帶 4-byte 長度 + 碼;之後 RELOC/INTERSEG 等重定位記錄)。
- 下一步:從 seg#2 body 抽出純碼 bytes(剝掉 OMF record 包裝),才能餵反組譯器。

> ⚠ ExpressLoad 段的 body 可能不是標準 LCONST+RELOC,而是「已重定位映像 + 壓縮重定位字典」。
> 需先確認 seg#2 body 第一個 record 型態,再決定抽碼方式。Step 2 處理。

## Step 2 — 抽出 seg#2 純碼

seg#2 body 第一 record = **LCONST(0xF2)**,長度 52722 = 整段碼。碼從檔案 **0x130** 起,52722 bytes;
之後是 0xF7(ExpressLoad 壓縮重定位)記錄。抽出 → `re_work/iigs/ULTIMAI_seg2.bin`(seg#3 同法 → `_seg3.bin`)。
入口(offset 0)= 標準 GS/OS 啟動:`PHK; PLB; TDC; STA $b69a; TSC; STA $b6a4; JSR $0039; ...`。

## Step 3 — 65816 反組譯器 + toolbox 呼叫盤點

寫 `tools/re/iigs/dis65816.py`(完整 256 opcode 表 + **M/X 旗標寬度追蹤**:REP/SEP 改 A、X/Y 8/16 寬,
決定 imm 運算元長度——這是 65816 比 6502 難的點)。
搜 `JSL $E10000`(toolbox dispatch,`22 00 00 e1`)→ 59 個 toolbox 呼叫。前面 `LDX #func`(`a2 lo hi`)取 function word
(= `call<<8 | tool`)。**Resource Manager(tool 0x1E)**:
- `0x0e1e` = **LoadResource** @0x22a ×1(錨點)
- `0x281e`(call 0x28)@0xd2,@0x16a;`0x171e`(call 0x17)@0x279

## Step 4 — type 0x0001 壓縮圖的「載入閘道」

反組譯 LoadResource 周邊(靜態溯源:caller 邏輯在主段),解出兩層 wrapper:

**`0x21e` = LoadResource 包裝**(mx=16-bit):
```
TSC;PHD;TCD              ; 建 stack-frame DP
PHA;PHA                  ; 預留 result handle 空間
PEI $05                  ; push resType(dp$05)
PEA $0000; PEI $03       ; push resID(高=0,低=dp$03)
LDX #$0e1e; JSL $e10000  ; ★ LoadResource(resType, resID)
PLX;PLY; STX $07;STY $09 ; handle → dp$07/$09
TAX; BNE err
LDY #4; LDA [$07],y; ORA #$8000; STA [$07],y  ; 設 resource offset 4 的 bit15(已載旗標)
DEY;DEY; LDA [$07]→X; LDA [$07],y→A           ; 讀 header 前 4 bytes(007d0100)
STX $07; STA $09; RTS                          ; 回傳 header word0/word1
```
⇒ 0x21e **只載入 + 讀 header(`007d0100`:word0=0x7d00, word1=0x0001)+ 設旗標**,**不解壓**。

**`0x201` = 「載入 type 0x0001」閘道**:
```
TSC;PHD;TCD; PHA;PHA
PEA $0001                ; ★ 硬寫 resType = 0x0001(壓縮圖類型)
PEI $03                  ; resID(dp$03)
JSR $021e                ; → LoadResource wrapper
... RTS
```
⇒ **0x201 = 「按 id 載入一個 type 0x0001 壓縮圖」**,全程式有 **11 個 caller**(各載不同圖):
@0x519, 0x53c, 0xdd5, 0xe5e, 0xea0, 0x1b24, 0x36b6, 0x6e43, 0x8a3f, 0xb250, 0xb38f。

**第一性結論(到此)**:壓縮圖經 `caller → 0x201(type=1) → 0x21e(LoadResource)` 載入;
**解壓不在載入路徑**,而在 caller 拿 handle 後的「繪製/解壓」碼。下一步:追一個 0x201 caller(如 0xdd5)
看它如何用 handle → 找解壓 loop(讀壓縮 byte、依 flag 高位 copy literal/run、寫 SHR 像素)。
