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
