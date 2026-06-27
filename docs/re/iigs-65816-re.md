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

## Step 5 — blit 迴圈(resource 像素 → SHR)

追 0x201 caller @0xdc9:載入後 `LDA [$05]` 讀 header word0,拆高/低 byte 當維度(height/width)。
@0xe20 是 blit 迴圈:`SEP #$21; LDY width-1; LDA [src],y; STA [dst],y; DEY; BPL`(每列 copy)、
`src += width; dst += 0xa0`(160=SHR 每線 bytes)、`DEC height; BNE`。
⇒ **把 resource 像素直接 copy 到 SHR(raw,非解壓)**。⇒ 載入時資料已是解壓好的 → 解壓在別處。

## Step 6 — ★★★ 破解:type 0x0001 = LZSS(Resource Converter @0x398)

啟動碼 @0xd2 用 **`ResourceConverter`(RM call 0x28)** 註冊:`PEA $0000; PEA $0398`(convertProc=0x398)、
`PEA $0001`(rType=0x0001)。⇒ **type 0x0001 載入時自動呼叫 0x398 解壓**(GS/OS resource converter 機制)。

反組譯 converter @0x398:讀 param block 指標 → `NewHandle`(MemMgr 0x0902)配輸出記憶體 → 解壓 loop @0x41c:
```
SEP #$21; LDA [src],y; STA $05        ; 讀控制 byte(8 bits,LSB first)
ROR $05; BCC match                     ; bit=0 → match;bit=1 → literal
  (literal) LDA [src],y; STA [dst]; DEX; INC dst         ; copy 1 byte
  (match @0x441) REP #$21; LDA [src],y  ; 讀 16-bit descriptor
    offset12 = desc & 0x0FFF; distance = 0x1000 - offset12
    length   = (desc >> 12) + 3
    從 dst[-distance] 回拷 length bytes                    ; LZ back-reference
DEX; BEQ done                          ; 輸出滿(dp$09 = 解壓大小)即止
```
**⇒ 格式 = 經典 LZSS:控制 byte(bit=1 literal / bit=0 match)、match = 12-bit offset(distance=0x1000−off)+ 4-bit length(+3)。
壓縮流從 resource offset 2 起。** 工具 `tools/re/iigs/lzss_decode.py`。

### ✅ 驗證成功
id1(4191B,header `00 7d 01 00`)從 offset 2 LZSS 解壓 = **正好 32000 bytes = 320×200 SHR 4bpp 一張螢幕**,
渲染 = **ORIGIN logo(「An Electronic Arts Company」)**,乾淨可辨!**IIgs 自訂圖格式完全破解。**
⇒ 79 個 type 0x0001 resource 全可解 → 含 overworld tileset → 可抽真實 IIgs 載具/怪物 tile(下一步)。

> 先前誤判「PackBytes 爆增」是因為 type3 假設錯;真實是 LZSS。第一性逐行反組譯(註冊 converter → 解壓 loop)
> 才拿到確切演算法。同 retro「反編當 oracle 不照抄」「斷言前先驗證」。

## Step 7 — 全 resource 解碼盤點(LZSS 套用)

`lzss_decode.py` 套到全部 79 個 type 0x0001(從 offset 2 解壓),確認格式對全體有效:
- **全螢幕 320×200(32000B)**:id01=ORIGIN logo、id02/0a/0e/32=Ultima 標題/「The Software Gremlins」/片頭/結局螢幕(渲染乾淨可辨)。
- **portrait/場景(128×64,4096B)**:id15-18。
- **HUD/分數表**:id08(17408B,寬 256 渲出「CURVE / BONUS / 76507」文字)。
- **小 sprite(812B×10)**:id1a–id24。
- 其餘多為非-320-寬子圖(對角條紋 = 寬度未對;各有自己維度,在 resource header 或 caller 設定)。

**overworld tileset 定位(下一步)**:tileset 的確切 resID + 像素維度需用靜態溯源追「地圖繪製碼」——
找哪個 0x201 caller 載入地圖 tile resource、它讀 header 取的寬高。11 個 caller(@0x519…0xb38f)逐一看 resID;
或對照 hg101 實機 tile 的結構在解碼輸出中比對。

**到此的成果**:**IIgs 自訂圖格式(type 0x0001 = LZSS)完全破解並驗證**,79 個圖 resource 全可解。
這解開了先前「載具/怪物 authentic tile」的牆——只差把正確的 tileset resource 對映到 engine 52 槽。
工具鏈:`omf_parse.py`(段)、`dis65816.py`(反組譯)、`lzss_decode.py`(解壓)。
