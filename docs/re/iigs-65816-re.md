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

## Step 8 — resource 分類 + resID 表(overworld tileset 仍待定位)

追 0x201 caller 用的 resID 表(`LDA $table,x`):
- `$b6c6`:(resID, val) 對 → resID 37-49(id25-id31)= 一組遊戲螢幕圖。
- `$3839`:resID 26-35(id1a-id23)= 10 個 812B 圖。
- `$ca83`:resID 21-24(id15-18)= portrait(128×64)。

**全 79 個 type 0x0001 解碼後分類**(LZSS 全可解):
| 類別 | resource |
|---|---|
| 全螢幕片頭/標題(32000B) | id01 ORIGIN、id02 Ultima標題、id0a/0e/32 等 |
| portrait(128×64) | id15-18 |
| HUD/分數(空戰) | id08(「CURVE / BONUS / 76507」) |
| **商店/選單 UI 文字** | id1a-id23(解出含「WEAPONS」「ME…」ASCII) |
| 遊戲螢幕圖 | id25-id31 等(各有關聯 palette,維度待定) |

⚠ **16×16 overworld tile(草/水/山…)不在以上明顯處**:當 16×16 tiles 渲染全雜訊;
seg#3(UltimaData)當 tiles 也雜訊。⇒ overworld tile 的儲存方式仍待定位:
可能在某 type 0x0001 resource 的特定 palette+維度未命中、或以 rIcon-style(16×16+mask)存於他處、
或繪圖走 QuickDraw/tile-engine 從 seg#3 特定 offset 取。

**下一步**:反組譯「overworld 地圖繪製主迴圈」(讀 map cell → 取 tile 圖 → blit),
從它實際取 tile 圖的指標反追到確切儲存位置 + 維度。LZSS 既破,拿到位置即可解。

**到此確立**:IIgs 圖格式(LZSS)完全破解、79 圖全解、resource 分類 + resID 表清楚;
唯 overworld 16×16 tile 的確切儲存待最後一步定位。

## Step 9 — 破解 sprite atlas 格式(找 overworld tile-draw 途中)

追 SHR blit routine(`ADC #$00a0`):三處皆填色/清屏;`0xdc9` = 「在 (x,y) 畫圖 resID」(組合選單/標題),4 散落 caller,非 tile loop。
找**常數 resID 的 0x201 載入**(`PEA $00XX; JSR $0201`):resID 8(id08 HUD)、13(id0d)、0(不存在)。

反組譯 `0x8a3a`:載入 **id0d** → handle 存 $8873;`0x8a52` = 「index → 指標」:`LDA [id0d],y`(y=index*2)取 offset → base+offset。
⇒ **id0d = sprite atlas**:開頭 **offset 表**(48 項)+ 各 sprite **`[width_bytes:1][height:1][4bpp 像素]`**。
驗證:sprite size = 下一 offset − 本 offset = `w*h + 2`(sprite2 w3h5=17 ✓、sprite4 w6h9=56 ✓)。
工具參數化解碼 → 渲出 **48 個太空戰鬥 sprite**(爆炸環、星球、太空梭/敵艦 TIE-fighter、爆炸),全乾淨可辨。

**⚠ overworld tile 仍未定位**:掃全部 79 resource 只有 id0d 是此 atlas 格式;overworld 16×16 tile 不在 atlas、
不在 id08(HUD)、不在標準 resource(當 tiles 渲染雜訊)。⇒ overworld 用**另一種 tile-draw 機制**(尚未反組譯到):
可能讀 map data + 從某處(未定位的 tile 陣列 / data 段特定 offset / 另一格式 atlas)blit。
**確切下一步**:反組譯 overworld 主畫面更新 routine(讀 map cell → 取 tile → blit 16×16),從取 tile 的指標反追。

**本輪成果**:LZSS + **sprite atlas 格式**雙破解;太空戰鬥 48 sprite 全解。overworld 地形 tile 待定位繪製 routine。

## Step 10 — 主事件迴圈 + 輸入層(追 overworld 重繪途中)

從 seg#2/seg#3 找 atlas/map:**都無**(seg2/3 既無 offset-table atlas,也無 64×64 低值 map 區)。
⇒ overworld tile 與 map 不以明顯結構嵌在程式段。

改從輸入錨點追:
- **GetNextEvent**(EM `0x0a06`)@0x2fc → 事件迴圈 0x2ea:檢查 keyDown(type 3)/autoKey(5)→
  key code 比對 + **keypad→方向 remap**(table @0x380 = `38 34 35 36 32`= 鍵盤碼 8/4/6/2 = 上左右下)。
- 取鍵 routine 0x2ea 的 caller:0x90b/0x91d/0x9da(選單/等鍵)、**0x785d/0x7873(文字輸入:取鍵+印字,如命名)**。
- 0x9da = 等鍵小迴圈。

**追到的層級**:OMF/段、resource 載入/converter/LZSS、sprite atlas、blit、主事件迴圈、取鍵、文字輸入。
**仍未到**:overworld 地圖主畫面更新 routine(讀 map cell → blit tile)在更深呼叫圖(輸入→命令 dispatch→move→重繪)。

**目前定論**:IIgs 圖形系統的兩個核心格式(**LZSS + sprite atlas**)已破解並驗證;79 圖全解、太空 48 sprite 全解;
但 overworld 16×16 地形 tile 的繪製 routine 與儲存位置藏得很深,需再聚焦一輪從「gameplay 命令 dispatch」往下追到重繪。
(此為單一格式 RE 的最後一哩,非格式問題——格式已破,差定位。)

## Step 11 — ★★★ 定位 overworld tile-draw + tile buffer

第一性:tile-draw 必經 SHR 位址計算。發現 **`$b70e` = SHR 列位址表**(值 0x2000+row×160,差 0xa0)。
追其用戶 → **`$0f75` = DrawTile**:用 `$c4df[tile_type]` 取 tile 圖 offset、`$b70e[row]` 取 SHR 列位址,
從 **`$004c00,x`** 讀 tile 像素寫 SHR。
另一 tile-draw `$06cb` 從 `$4c00,x` 直拷整個 tile(標準 4bpp)到螢幕緩衝。

**overworld 地圖主迴圈 `$196f`**:
```
row=0x10..0x90 step2:  col=0x10..0x90 step2:
  tile_type = map_byte[$01]; $01++      ; 讀 64×64 map data
  DrawTile(tile_type, col, row) → $0f75
```
**`$c4df` 表 = tile offset:[0,128,256,…,6144]**(差 128)⇒ **48 個 tile × 128 bytes = 16×16 4bpp,在 `$4c00` buffer**。

### tile buffer 抽取嘗試(MAME dump)
`$4c00` 是 bank-0(或重定位)RAM buffer,啟動時填入。MAME 在建角(到 350s)dump:
- bank-0 `$4c00` 全 0;banks $01-$05 的 `$4c00` 是碼/雜訊;tile-likeness 掃 320KB dump 找到 bank$01:8c00 = font/UI(非地形)。
⇒ **overworld 地形 tile 在建角時尚未載入**,**進 overworld 才解壓/展開填入 `$4c00`**。

**結論(本輪重大突破)**:**完全定位 overworld tile-draw 機制與 tile buffer**——
48 個 16×16 tile 在 `$4c00`(`$c4df` 索引),由 `$0f75`/`$06cb` 繪製,主迴圈 `$196f` 讀 64×64 map。
**唯一剩**:tile 像素在進 overworld 才填入 → 抽取需 ① MAME 進 overworld dump `$4c00`(卡 GS/OS 滑鼠對話框),
或 ② 反組譯 overworld-init 找填 `$4c00` 的來源(resource/seg#3 packed)+ 展開法。tile 數量(48)、尺寸(16×16×128B)、
索引($c4df)、繪製 routine 全已知——格式與機制全破,差最後一步取像素。

## Step 12 — tile 來源靜態追蹤(本輪結論)

走純靜態路找填 `$4c00` 的來源(繞過滑鼠牆):
- **掃 seg#3 + 解壓 resource 找 48-tile 色少區**:seg#3 無;resource 只命中全螢幕圖的色少背景(假陽性,非 tileset)。
  ⇒ tile 源**非簡單靜態 48-tile 連續區塊**。
- **找 `$4c00` 寫入/指標**:無 `STA $4c00,x`、無 `LDA #$4c00` 指標設定 ⇒ 填充非直接 store loop。
- **找 map-draw `$196f` / tile-copy `$06b8` 的 caller**:無直接 JSR ⇒ 經 **JSL(跨 bank)或跳表 `JMP ($0e7f,x)`** dispatch。

**本輪結論**:overworld tile-draw 機制 100% 反組譯(`$196f` 主迴圈 → `$0f75` DrawTile → `$4c00` 48-tile buffer,`$c4df` 索引);
但 **tile 像素在「進 overworld」時才填入 `$4c00`**,填充來源/展開法經 JSL/跳表 dispatch,單純靜態掃描未命中。
**取得真實 tile 像素的兩條路**(下一階段):① MAME 進 overworld dump `$4c00`(卡 GS/OS 滑鼠對話框);
② 追跳表 `$0e7f` 的 overworld 命令 → init → 填 `$4c00` 來源。機制全破,差最後取像素。
