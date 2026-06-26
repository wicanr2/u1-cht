# E2 Apple IIgs 素材抽取 — woz 牆已破(2026-06-27)

## ★★ 突破:woz 牆用 floptool 破解(不必自寫 GCR 解碼器)

素材:`re_work/Ultima I IIgs.woz`(WOZ2,3.5" 800K,2 面,Applesauce v2.01,sync=1/cleaned=1)。
1994 Heineman/Vitesse IIgs 移植(GS/OS app,ProDOS volume `ULTIMA.I`,建檔 1994-11-03)。

### 關鍵:MAME `floptool` 內建 WOZ↔ProDOS 編解碼器
- docker image `u1-a2`(`docker/Dockerfile.a2` = u1-re + `mame-tools`)。
- `floptool flopdir  woz prodos <woz>` → 列目錄。
- `floptool flopread woz prodos <woz> <path> <out>` → 抽檔(data fork → `out`;**resource fork → `._out` AppleDouble**)。
- ⇒ **先前文件記載的「自寫 3.5" GCR 6-and-2 解碼器」需求作廢** —— floptool 直接給邏輯檔。

### Volume 內容
| 檔 | data | rsrc | 內容 |
|---|---|---|---|
| `ULTIMAI` | 90621 | **579766** | GS/OS app;圖素/字串在 **resource fork** |
| `FINDER.DATA` | 59 | — | Finder metadata |

## resource fork 解析(`tools/re/iigs/extract_woz.py`)
- AppleDouble(`._ULTIMAI`,magic `00051607`)→ 取 resource fork(floptool 的 entry offset 不標準,
  工具改用「掃 offset 選能成功解析 IIgs map 者」,實測 fork @0x2a)。
- IIgs Resource Manager:header(rFileVersion/rFileToMap/rFileMapSize)→ map → 110 個 reference record
  (各 20 byte:type/id/offset/attr/size/handle)。

### resource 類型盤點(110 個)
| type | 數 | bytes | 性質 |
|---|---|---|---|
| **0x0001** | 79 | 324K | 自訂:**圖像(PackBytes 壓縮)+ palette(32B=16×$0RGB)交錯** |
| **0x8024** | 18 | 252K | 較大圖像/螢幕(待確認格式) |
| 0x8001 rIcon | 4 | 672 | ✅ **已解**:城堡 icon + 騎士 icon(16×16/8×8) |
| **0x8015** | 2 | 261 | 遊戲/系統文字字串(翻譯用) |
| 0x8005/06/13/29/2a/2b | 各 1-2 | — | UI(選單/視窗/字串) |

## ✅ 已確認的 IIgs 圖像格式(rIcon 驗證)
- **4bpp,row-major,ceil(w/2) bytes/row,高 nibble = 左像素**(渲 rIcon 出城堡+持劍騎士,palette 對)。
- rIcon = `type,size,h,w`(各 word)+ image[h*ceil(w/2)] + mask。
- palette = 16 × 12-bit `$0RGB`(word LE);type 0x0001 內每幾個圖像配一個 32-byte palette resource。
- 大宗圖像(0x0001/0x8024)為 **Apple PackBytes 壓縮**(從 byte 0 起,`unpackbytes()` 已實作:
  type0 literal / type1 單byte×c / type2 4byte群×c / type3 單byte×4c)。

## 🔄 剩餘(overworld tileset 抽取)
woz 牆雖破,但 **IIgs 1994 Vitesse 是增強版自訂美術**,tileset 抽取是更深的格式 RE:
- PackBytes 解壓後,各圖像**維度不一**(非固定 16×16 strip);raw 每 resource header 不同
  (id1 `007d0100…`、id1a `2c03…`)→ 需逐 resource 定維度。
- 試 width 320 為橫紋雜訊;width 64/80 出現縱向結構但未成清晰 tileset。
- 增強美術未必對得上 engine 52-slot overworld 模型(可能是全螢幕場景/portrait)。
- **下一步候選**:① 精修 PackBytes + 從 header 取每圖維度 → 渲染辨識哪些是 16×16 tile;
  ② 對照 KEGS/GSplus 模擬器跑 overworld 截圖當 ground truth(同 MSX openMSX 法);
  ③ 若美術不合 52-slot,改抽「最接近 DOS 風格」的子集或放棄,優先做 E4/E5。

## 工具 / 環境
- `docker/Dockerfile.a2`(u1-a2:floptool)、`tools/re/iigs/extract_woz.py`(AppleDouble+resource 解析+dump)。
- 抽出物在 `re_work/iigs/`(gitignore)。

## 替代順序
- E4 PC-98(`.fdi`)、E5 Atari(ATR,6502 已反組譯)是標準磁區映像,抽檔較直接,圖格式較單純 → 可優先。
