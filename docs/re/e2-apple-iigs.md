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

## ★★ MAME 模擬器路:管線已建好(2026-06-27)

GS ROM 取得後(memory `retro-bios-source`,Abdess/retrobios `apple2gs.rom/.zip`),改走模擬器 ground truth:
- **MAME 在 apt**(0.264,先前漏 `apt-get update` 才看不到)→ `docker/Dockerfile.mame`(u1-a2 + mame + xvfb)。
  binary 在 `/usr/games/mame`(412MB)。`apple2gs.zip`(放 `re_work/iigs/bios/`)`-verifyroms` = good。
- **跑**:`mame -rompath <bios> apple2gs -flop3 <woz> -autoboot_script <lua> -video none -seconds_to_run N`。
  woz = 3.5"(flop3/flop4 吃 .woz)。lua dump bank $E1 `$2000–$9FFF`(`0xE12000–0xE19FFF`)= SHR 螢幕。
- ✅ **SHR dump + 渲染管線完成**:`tools/re/iigs/mame_dump.lua`(dump)+ `render_shr.py`(SCB+16 palette 正確解 12-bit $0RGB)。
- ✅ MAME 開機正常(顯示 IIgs ROM 開機畫面)。

### ⛔ 卡點:遊戲碟非自我開機 → 需 GS/OS 系統碟
- MAME 開機畫面顯示 **「UNABLE TO LOAD PRODOS」**:woz volume `ULTIMA.I` **只有 `FINDER.DATA` + `ULTIMAI`,
  無 PRODOS / System 系統檔** → 是 GS/OS **應用程式碟**,不能單碟自我開機。
- 需 **GS/OS System 6.0.1 開機碟(.2mg/.po)** 放 flop3、Ultima woz 放 flop4,開機 GS/OS 後從 Finder 啟動 ULTIMAI
  (Finder 啟動 app 需**滑鼠**注入或設 startup app)→ 進遊戲 → 建角 → overworld → dump SHR → 切 16×16 tile。
- 本 session 快速下載 GS/OS 系統碟未果(asimov/archive.org 多為錯誤頁或限流)。**待使用者提供系統碟**
  (放 `re_work/iigs/sys/`),管線即可一路跑完。

### 剩餘步驟(系統碟到位後)
1. GS/OS 開機 + 啟動 ULTIMAI(滑鼠注入,或改造開機碟設 ULTIMAI 為 startup app 免 Finder)。
2. lua 注入鍵盤/滑鼠導航到 overworld(同 MSX openMSX 法,event-driven)。
3. dump overworld SHR → 切 16×16 tile → 對映 engine 52 槽 → `build_*_pack.py` 出 PNG。
   (SHR 是已渲染畫面;tile 直接從 framebuffer 切,繞過 type 0x0001 自訂壓縮)。

### 建議
- IIgs 管線就緒,**只差 GS/OS 開機碟**;取得後可完成。
- 平行可先做 E4 PC-98 / E5 Atari(標準磁區映像、格式單純)。

## 工具 / 環境
- `docker/Dockerfile.a2`(u1-a2:floptool)、`tools/re/iigs/extract_woz.py`(AppleDouble+resource 解析+dump)。
- 抽出物在 `re_work/iigs/`(gitignore)。

## 替代順序
- E4 PC-98(`.fdi`)、E5 Atari(ATR,6502 已反組譯)是標準磁區映像,抽檔較直接,圖格式較單純 → 可優先。
