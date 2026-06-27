# Apple IIgs 版《創世紀 I》逆向工程全紀錄

> 1994 年 Vitesse / Rebecca Heineman(《Bard's Tale III》《異星搜奇》班底)為 Apple IIgs 重製的《Ultima I》,
> 是公認畫面最強的版本——超高解析、synthLAB 音樂。但發行商旋即倒閉,此版**極其稀有**,
> 網路上乾淨截圖屈指可數,圖檔格式從無公開文件。
>
> 本章記錄把這個版本**從一張 woz 磁碟映像,一路逆向到完全破解其自訂圖形壓縮格式**的完整過程。
> 這是一條走了好幾道牆、最後靠第一性原理逐行反組譯打通的路。
> 細部步驟日誌另見 [`iigs-65816-re.md`](iigs-65816-re.md)(反組譯)、[`apple2gs-emulation-experience.md`](apple2gs-emulation-experience.md)(模擬器)、[`e2-apple-iigs.md`](e2-apple-iigs.md)(E2 進度)。

---

## 0. 一頁總結

| 關卡 | 解法 | 工具 |
|---|---|---|
| woz 磁碟(flux/GCR) | MAME `floptool` 直接 woz→ProDOS,**免自寫 GCR 解碼器** | `u1-a2` docker |
| 抽出遊戲檔 | data fork + resource fork(AppleDouble) | `extract_woz.py` |
| 解析 resource fork | IIgs Resource Manager map,110 個 resource | `extract_woz.py` |
| 圖格式(type 0x0001) | **❌ 靜態解不出 / 模擬器卡滑鼠對話框 / 截圖非精確 palette** | (三面牆) |
| **★ 圖格式破解** | **反組譯 ULTIMAI(65816)→ 找到 LZSS 解壓 converter** | `omf_parse / dis65816 / lzss_decode` |
| overworld 地形 tile | hg101 原生 SHR 截圖直接切(繞過格式) | `build_iigs_pack.py` |

**最終結果**:type 0x0001 = **LZSS 壓縮**(12-bit offset + 4-bit length)。79 個圖 resource 全可解,
驗證解出 ORIGIN logo、Ultima I 標題等。先前「載具/怪物拿不到真實素材」的牆,被反組譯打通。

---

## 1. 破第一道牆:woz 磁碟抽檔

素材 `Ultima I IIgs.woz`(WOZ2,3.5" 800K,Applesauce v2.01)是 **flux/bit-stream**,3.5" 用 GCR 6-and-2 編碼。
先前評估說「需自寫 3.5" GCR 解碼器」。**其實不必**——MAME 的 `floptool` 內建 WOZ↔ProDOS 編解碼:

```bash
floptool flopdir  woz prodos "Ultima I IIgs.woz"             # 列目錄
floptool flopread woz prodos "Ultima I IIgs.woz" ULTIMAI out # 抽檔(rsrc fork → ._out)
```

Volume `ULTIMA.I` 只有兩個檔:`FINDER.DATA` 與 **`ULTIMAI`**(data fork 90621B + **resource fork 579766B**)。
⇒ 圖素與程式都在 ULTIMAI。

## 2. 解析 GS/OS resource fork

`._ULTIMAI` 是 AppleDouble(magic `00051607`)→ 取出 resource fork →
解 **IIgs Resource Manager** 結構(header → map → 110 個 reference record,各 type/id/offset/size)。
類型盤點:

| type | 數 | 性質 |
|---|---|---|
| **0x0001** | 79 | **自訂壓縮圖 + palette(32B=16×$0RGB)交錯** ← 目標 |
| 0x8024 | 18 | 音效(8-bit PCM,Ensoniq DOC 256-byte page,渲成灰階呈波形) |
| 0x8001 rIcon | 4 | 未壓縮 4bpp,渲出城堡 + 持劍騎士 |
| 0x8015 | 2 | 遊戲文字字串(翻譯用) |

## 3. 三道牆:為什麼圖格式一開始解不出

type 0x0001 是圖,但用盡靜態與動態手段都卡住:

1. **靜態猜格式**:當成 raw 4bpp / 各種 PackBytes 變體解壓,4191B 的圖爆增到 90K–200K(荒謬)、
   自相關無 row 週期、渲染全雜訊。⇒ 是某種非標準自訂壓縮。
2. **模擬器 ground truth**(MAME + GS/OS):跑遊戲到 overworld dump SHR 螢幕直接切 tile——
   開機、鍵盤自動導航到角色創建都通了,但存/讀檔走 **GS/OS 滑鼠檔案對話框**,
   而 MAME lua 的滑鼠注入 `set_value` 對相對軸非乾淨座標 → 點不準 → 進不了 overworld。
3. **實機截圖 Rosetta**:hg101 截圖**色彩非精確 IIgs palette**(非 ×17 倍數)→ 無法當解碼 key。

> 三面牆都不是「不可能」,而是「此路成本高」。真正的解法是**直接反組譯程式**問它怎麼解壓。

## 4. ★ 打通:反組譯 ULTIMAI(65816)破解圖格式

第一性原理:既然程式自己會解壓,那解壓演算法就在程式碼裡。逐步反組譯(完整日誌見 `iigs-65816-re.md`):

1. **檔案格式**:ULTIMAI = GS/OS **OMF**(ExpressLoad)。主程式碼 = seg#2(52722B 65816)。
2. **65816 反組譯器**:自寫 `dis65816.py`(256 opcode + **M/X 旗標寬度追蹤**——REP/SEP 改 A、X/Y 8/16 寬,
   是 65816 比 6502 難的點)。
3. **找 toolbox 呼叫**:搜 `JSL $E10000`(`22 00 00 e1`)→ 59 個。其中 Resource Manager 的 **LoadResource**($0E1E)。
4. **追載入閘道**:`0x201` wrapper 硬寫 `PEA $0001`(resType=0x0001)→ 是「載入壓縮圖」的入口(11 caller)。
5. **找解壓位置**:caller 載入後用 blit 迴圈把像素 raw copy 到 SHR ⇒ 解壓不在載入路徑。
6. **★ 命中**:啟動碼用 **`ResourceConverter`(RM call 0x28)註冊一個 converter 在 offset 0x398**,綁 type 0x0001。
   GS/OS 載入該類型時自動呼叫它解壓。反組譯 0x398:

```
; converter @0x398:LZSS 解壓
SEP #$21; LDA [src],y; STA $05    ; 讀控制 byte(8 bits,LSB first)
ROR $05; BCC match                 ; bit=0 → match;bit=1 → literal
  (literal)  copy 1 byte src→dst
  (match)    REP; LDA [src],y       ; 讀 16-bit descriptor
             offset12 = desc & 0x0FFF;  distance = 0x1000 - offset12
             length   = (desc >> 12) + 3
             從 dst[-distance] 回拷 length bytes
DEX; BEQ done                       ; 輸出滿即止
```

⇒ **type 0x0001 = 經典 LZSS**:控制 byte 逐位(1=literal、0=match);match = 12-bit 回溯距離 + 4-bit 長度(+3)。
壓縮流從 resource **offset 2** 起。

### 驗證
`lzss_decode.py` 解 id1(4191B)→ 正好 **32000 bytes = 320×200 SHR 一張螢幕** → 渲染 = **ORIGIN logo**,乾淨可辨。
套到全部 79 個 resource 全可解:Ultima I 標題、「The Software Gremlins」片頭、portrait、HUD 分數表…

## 5. 繞道也有用:實機截圖直接切地形 tile

在反組譯打通前,先用 hg101 的 **320×200 原生 SHR、PNG 無損**截圖直接切了 8 個真實 overworld tile
(水/草/森林/山/城堡/城鎮/玩家/馬,offset (8,8) 網格),配 EGA fallback 做出可玩的 `iigs.png`。
教訓:卡格式/模擬器時,**先找原生解析度實機參考圖**往往最快(memory `retro-native-screenshot-tile-rip`)。

## 6. 成果與工具鏈

**成果**:Apple IIgs 版《創世紀 I》的自訂圖形壓縮格式(type 0x0001 = LZSS)**完全破解並驗證**;
79 個圖 resource 全可解;overworld 地形 tile 已 authentic(截圖切),載具/怪物待從解壓的 tileset resource 抽出。

**工具鏈**(`tools/re/iigs/`):
| 工具 | 作用 |
|---|---|
| `omf_parse.py` | 解 GS/OS OMF 段表 |
| `extract_woz.py` | AppleDouble + IIgs resource fork 解析 + dump |
| `dis65816.py` | 65816 反組譯器(M/X 旗標寬度追蹤) |
| `lzss_decode.py` | type 0x0001 LZSS 解壓器 |
| `render_shr.py` | SHR(SCB + 16 palette 12-bit $0RGB)→ PNG |
| `mame_*.lua` | MAME apple2gs 開機 + 導航 + SHR dump |
| `build_iigs_pack.py` | 實機截圖切 tile → 832×16 PNG |

**環境**:`docker/Dockerfile.{a2,mame}`(floptool / MAME);BIOS 與 woz / GS-OS 碟在 `re_work/iigs/`(版權,gitignore)。

## 7. 追地圖繪製碼:overworld tile-draw 機制調查

LZSS 既破,接著反組譯找 overworld 的逐格 tile-draw,定位 tile 圖確切儲存。調查結果(細節見 `iigs-65816-re.md` Step 8-9):

- **resource 分類(79 個全解)**:片頭/標題(32000B 全螢幕)、portrait(128×64)、HUD 空戰分數(「BONUS/CURVE」)、
  **商店選單 UI 文字**(解出「WEAPONS」)、遊戲螢幕圖。**16×16 overworld 地形 tile 不在這些明顯處**。
- **resID 表**:`$b6c6`(resID 37-49)、`$3839`(26-35 商店 UI)、`$ca83`(21-24 portrait)把 resource 分組。
- **SHR blit routine**:`ADC #$00a0`(SHR 每線 160)三處——0xaaf=矩形填色、0xc39=清屏、
  **0xdc9 = 「在 (x,y) 畫圖 resID」**(`PEA x;PEA y;PEA resID;JSR $0dc9`),4 個散落 caller、組合選單/標題畫面用,
  **非逐格 tile loop**。
- ⇒ **overworld tile-draw 是尚未定位的另一 routine**。最可能:tiles 在啟動時 LZSS 解壓進一塊 **RAM tile-cache**,
  地圖主迴圈讀 map cell → 從 cache blit 16×16。需找這個迴圈(讀 map data + tile-cache 指標)反追到 cache 來源 resource。

**現狀**:`iigs.png` = 8 個真實 IIgs 地形/玩家/馬(hg101 截圖切)+ EGA fallback,可玩。
完整 52 槽真實素材就差「定位 overworld tile-cache 來源 resource + 維度」這一步——格式既破,拿到位置即可解。
