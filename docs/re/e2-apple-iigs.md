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

## 🔄 剩餘(overworld tileset 抽取)— 二級牆:自訂壓縮圖格式

深入嘗試後(2026-06-27),tileset 卡在 **type 0x0001 的自訂壓縮圖格式**,以下為已排除/已確認,避免重蹈:

### 已確認(別再試)
- **type 0x8024 = 音效,非圖素**:`(size−10)/header[2] = 256` 恆成立 → 10-byte header + N×256-byte page;
  渲成 8bpp 灰階呈**波形**(Ensoniq DOC 8-bit PCM,256-byte page 對齊)。18 個 = 各音效。
- **type 0x8001 rIcon = 未壓縮 4bpp**(✅ 渲出城堡+騎士),但只有 4 個 app icon,非 tileset。
- **type 0x0001 圖像 = 自訂壓縮,非標準 PackBytes**:
  - 4 種 PackBytes 變體(t2/t3 ×4 或 ×1、skip 0/4 header)解壓,4191B 的 id1 爆增到 90K–200K(荒謬)。
  - 解壓後自相關**無清晰 row 週期**(峰值單調落在 8/12/16…= packbytes 4-byte group 假象,非真實 stride)。
  - 渲 width 320 為橫紋雜訊;width 64/80 有縱向結構但不成 tile。
  - raw header 每 resource 不同(id1 `007d0100`、id1a `2c03…`、id15 `0010e100`)→ 非統一格式。

### 兩條真正可行路(都需額外資源)
| 路 | 作法 | 阻礙 |
|---|---|---|
| **格式 spec** | 找 Vitesse/Heineman IIgs U1 圖格式文件,或反組譯 ULTIMAI data fork(OMF 65816)的繪圖碼 | 無公開 spec;65816 反組譯工程量大 |
| **模擬器 ground truth** | MAME `apple2gs` / KEGS / GSplus 跑遊戲到 overworld,dump SHR 螢幕($E12000)→ 直接切 16×16 tile(同 MSX openMSX 法) | **MAME 不在 apt、Apple IIgs ROM 不在 archtaurus/RetroPieBIOS** → 需另尋 GS ROM |

### 建議
- **優先做 E4 PC-98 / E5 Atari**(標準磁區映像、圖格式單純,較可能像 MSX 做出完整 pack)。
- IIgs 待「取得 GS ROM 跑模擬器」或「找到圖格式 spec」再回攻;woz 牆已永久破除,抽檔管線就緒。

## 工具 / 環境
- `docker/Dockerfile.a2`(u1-a2:floptool)、`tools/re/iigs/extract_woz.py`(AppleDouble+resource 解析+dump)。
- 抽出物在 `re_work/iigs/`(gitignore)。

## 替代順序
- E4 PC-98(`.fdi`)、E5 Atari(ATR,6502 已反組譯)是標準磁區映像,抽檔較直接,圖格式較單純 → 可優先。
