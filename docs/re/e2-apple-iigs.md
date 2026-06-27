# E2 Apple IIgs 素材抽取 — woz 牆已破(2026-06-27)

> ⚠ **後續重大更新(2026-06-27):type 0x0001 圖格式已破解 = LZSS**。
> 本檔下方「自訂壓縮抗拒 / 二級牆 / 需反組譯解壓常式」等是**破解前的歷史紀錄,結論已被取代**。
> 完整成果與最終結論見 **[`apple-iigs-reverse-engineering.md`](apple-iigs-reverse-engineering.md)**(整合章節)
> 與 [`iigs-65816-re.md`](iigs-65816-re.md)(反組譯逐步日誌)。本檔保留作 E2 探索過程紀錄。

## ★★★★ 最終解:用 hg101 實機截圖直接切 tile(2026-06-27)

**繞過全部難題**(模擬器導航 + 自訂壓縮格式):hg101 的 Apple IIgs 截圖是 **320×200 原生 SHR、PNG 無損**
(`reference/hg101/imgs/ultima1-appleIIgs-{07,12,10}.png`),每 tile 正好 16×16,直接切。
- overworld tile 網格相位 **offset (8,8)**(玩家居中推得)。
- 截圖 07(overworld)→ 水/草/森林/城堡/玩家;12(overworld)→ 山/城鎮/馬;10 = 城鎮內裝(另用)。
- 取得 **8 個真實 IIgs overworld tile**:水、草、森林、山、城堡、城鎮、玩家、馬。
- `tools/build_iigs_pack.py`:切 tile + 以 EGA 補未涵蓋槽(signpost/dungeon/載具/怪物)→ `assets/tilesets/iigs.png`
  (player/horse 去草地背景轉透明)。**game tester 驗證 overworld 渲染正常**(藍波水/綠草/森林/紅衣玩家)。
- ⇒ E2 主要 overworld tile **完成**。完整 52 槽(載具/怪物截圖未涵蓋)需 MAME 跑到 overworld dump(下方),
  或找更多實機截圖。

> 教訓:卡在模擬器導航/格式 RE 時,**先看有沒有現成實機參考圖**(原生解析度 PNG = 可直接切 tile),
> 往往比硬解格式或自動化模擬器快得多。同 retro 重寫「反編當 oracle 不照抄」「先建可驗證訊號」。


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

> ✅ **此牆已破(後續)**:type 0x0001 = **LZSS**(反組譯找到 ResourceConverter @0x398),79 圖全可解。
> 下方為破解前的試誤紀錄(保留作避免重蹈);最終結論見整合章節。「非標準 PackBytes」判斷正確——實為 LZSS。

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

## ★★★ GS/OS 開機 + 遊戲導航(2026-06-27,系統碟到位)

GS/OS System 6.0.2 開機碟(archive.org `IIGSSystem6.0.22mgDisks`,WebSearch 找到)放 `re_work/iigs/sys/`。
- **開機**:`-flop3 "…Disk 2 System disk.2mg" -flop4 "Ultima I IIgs.woz"` → GS/OS 6.0.2 開到 Finder 桌面
  (System Disk + Ultima I 兩碟掛載)。GS/OS 開機慢(~150-270s emulated,變異大)。
- **鍵盤自動導航**(lua `tools/re/iigs/mame_nav_chargen.lua`,ADB 鍵盤會緩衝,早送也生效):
  Finder type-select 'u' + Cmd+O ×2 → 啟動 ULTIMAI → 過 MidiSynth 警告 → 主選單 A → **完整角色創建**
  (屬性分配 Right/Down、race/sex/class 字母選、name 打字、Save? Y)。**全程鍵盤可驅動,已驗證到存檔對話框。**
- **SHR dump 快路**:`-video none`(快 ~8×,~90s/run)+ lua dump bank $E1 → `render_shr.py` 自己渲染,免 xvfb。

### ⛔ 最後一哩卡點:GS/OS 滑鼠對話框
ULTIMA I IIgs 的存檔/讀檔走 **GS/OS 標準檔案對話框**(SFPutFile / SFGetFile,640 模式):
- 存檔「Save thy game as…」:Return 偶可按 Save(時序敏感)→ 回主選單。
- 讀檔「Which game my Lord?」(主選單 B):**需滑鼠雙擊存檔檔**。鍵盤 Down/Return/type-select 皆無法選檔開啟 → 退回主選單。
- 滑鼠機制可用(ADB 相對:`MOUSE0/1/2`,`set_value(delta)` 每 frame 累加;**pin 到角落 X=-30×60f 已驗證**),
  但移到檔案/按鈕需校準 delta→pixel 比例(640 QuickDraw 座標),且存+讀兩個對話框各需校準 → 多輪迭代(每輪~90s)。

### 剩餘步驟(接續)
1. 校準滑鼠 delta→pixel,雙擊讀檔對話框的存檔 → 進 overworld(或滑鼠點 Save 按鈕過存檔對話框)。
   - 替代:找 Ultima I IIgs **現成存檔檔**放碟上、或改造碟設 ULTIMAI auto-launch 免 Finder。
2. dump overworld SHR → `render_shr.py` → 切 16×16 tile → 對映 engine 52 槽 → `build_*_pack.py` 出 PNG。
   (SHR 是已渲染畫面,tile 直接從 framebuffer 切,**繞過 type 0x0001 自訂壓縮**)。

### 環境 / 工具(本輪新增)
- `docker/Dockerfile.mame`(u1-mame:mame 0.264 + xvfb;mame binary 在 `/usr/games/mame`)。
- `tools/re/iigs/mame_dump.lua`(SHR dump)、`mame_nav_chargen.lua`(開機+角色創建導航)、`render_shr.py`(SHR→PNG)。
- BIOS `re_work/iigs/bios/apple2gs.{rom,zip,chr}`(memory `retro-bios-source`);系統碟 `re_work/iigs/sys/`。

### 建議
- IIgs **就差滑鼠校準過 GS/OS 對話框**(或現成存檔);其餘管線全通。屬可完成但需數輪迭代的收尾。
- 平行可先做 E4 PC-98 / E5 Atari(標準磁區映像、格式單純,較快)。

## 工具 / 環境
- `docker/Dockerfile.a2`(u1-a2:floptool)、`tools/re/iigs/extract_woz.py`(AppleDouble+resource 解析+dump)。
- 抽出物在 `re_work/iigs/`(gitignore)。

## 替代順序
- E4 PC-98(`.fdi`)、E5 Atari(ATR,6502 已反組譯)是標準磁區映像,抽檔較直接,圖格式較單純 → 可優先。

## ★ 載具/怪物 authentic tile:窮盡後的牆(2026-06-27)

地形/玩家/馬已 authentic(hg101 截圖切),但**載具/怪物 hg101 沒拍到** → 想補真實 IIgs 載具/怪物,試遍以下全卡:
1. **Rosetta Stone(用已知 tile 反推壓縮)**:hg101 截圖**色彩非精確 IIgs palette**(grass 色 `(18,82,0)` 非 ×17 倍數)
   → 無法把截圖色映射到 palette index → 無法當 Rosetta key。MAME SHR dump 雖精確 palette,但拍到的是文字選單非 tile。
2. **未壓縮 tile 搜尋**:type 0x0001 各 resource 當 raw 4bpp 渲染全雜訊 → tiles 確在自訂壓縮裡,非未壓縮。
3. **MAME 跑到 overworld**:鍵盤可通到角色創建,但存/讀檔走 **GS/OS 滑鼠檔案對話框**。
   - **lua 滑鼠注入失效**:`MOUSE_X/Y field:set_value()` 對相對軸**不是乾淨座標設定**——
     設常數小值游標貼角落、設 400/150/550 游標位置非單調(疑值域 wrap / scaled delta),**無法精準點檔案/按鈕**。
   - ⇒ 讀檔對話框過不去 → 到不了 overworld → 拿不到 authentic 載具/怪物 tile。

**結論(破解前)**:IIgs 載具/怪物 **EGA fallback**(`iigs.png` 已可玩)。
原列三條深坑:① MAME 滑鼠注入;② 找精確-palette 截圖;③ 反組譯 65816 找解壓常式。

> ✅ **更新(後續完成)**:走了 ③ —— **反組譯 ULTIMAI(65816)成功破解圖格式 = LZSS**,79 圖全可解,
> 並進一步定位 overworld tile-draw(`$196f`→`$0f75`)與 **48-tile `$4c00` buffer**(`$c4df` 索引,16×16×128B)。
> 機制全破;唯 tile 像素在進 overworld 才填入 `$4c00`,取得真實 tile 仍需進 overworld dump 或追 init 填充來源。
> 完整見 **[`apple-iigs-reverse-engineering.md`](apple-iigs-reverse-engineering.md)** §4/§7 與 `iigs-65816-re.md` Step 1-12。
