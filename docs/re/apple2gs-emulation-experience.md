# 在 Docker 裡用 MAME 跑 Apple IIgs + 自動導航 GS/OS 遊戲 + dump SHR — 完整經驗

> 緣由:E2 把 Ultima I 的 **Apple IIgs(1994 Vitesse)** 版 tile 抽出。woz 牆破了、資源 fork 解析了,
> 但圖素是 **type 0x0001 自訂壓縮**(非標準 PackBytes,靜態解不出)。改走「模擬器 ground truth」——
> 用 MAME 跑遊戲到 overworld、dump SHR 螢幕直接切 tile(同 E3 MSX 的 openMSX 法)。
> 本檔記錄完整流程 + 踩雷,可複用於任何「Docker 裡跑 IIgs / GS/OS app 並自動化」。
> 配 `e2-apple-iigs.md`(E2 進度)、`openmsx-build-experience.md`(MSX 對照)。

---

## 0. TL;DR / 最終結論

1. **MAME 在 apt**(0.264),不必源碼編;binary 在 `/usr/games/mame`(412MB),非 PATH。
2. **woz 抽檔不必自寫 GCR 解碼器** —— MAME `floptool` 內建 WOZ↔ProDOS。
3. **遊戲碟通常非自我開機**(GS/OS app),需 **GS/OS 系統碟**(System 6.0.x)放 flop3 開機、遊戲 woz 放 flop4。
4. **鍵盤可全自動**(lua + ADB ioport)把遊戲驅動過 Finder 啟動 → 角色創建;**ADB 鍵盤會緩衝**早送也生效。
5. **SHR 螢幕 = bank $E1 的 $2000–$9FFF**,lua dump 32KB 自己渲染(SCB + 16 palette,12-bit $0RGB)。
6. ⚠ **GS/OS 標準檔案對話框(存/讀檔)只認滑鼠**;鍵盤無法選檔。滑鼠 pin 到角落可行,但 delta→pixel 校準麻煩。
7. ★ **最快的路常常是繞過模擬器**:找**原生解析度實機截圖**(hg101 320×200 SHR PNG)直接切 tile。見 §9。

---

## 1. MAME:取得與 apple2gs ROM

- `apt-get update && apt-get install -y mame`(先 `update`,否則 `apt-cache policy mame` 顯示無候選 → 誤以為沒有)。
  binary 落在 **`/usr/games/mame`**(Debian 把 game 放 `/usr/games`,不在 PATH);ROM 目錄 `/usr/share/games/mame/roms`。
- **GS ROM**:`github.com/Abdess/retrobios` 的 `/bios/Apple/Apple II/`(memory `retro-bios-source`):
  `apple2gs.rom`(256KB,ROM 03,sha1 `bc32bc0e…`)、`apple2gs.zip`(MAME romset)、`apple2gs.chr`。
- 驗證:`mame -rompath <bios_dir> apple2gs -verifyroms` → `romset apple2gs is good`(用 `apple2gs.zip`)。
- 機器資訊:`mame apple2gs -listmedia`(flop1/2 = 5.25,**flop3/4 = 3.5"**,吃 `.woz/.2mg/.po/.dsk`)。

## 2. woz 抽檔(floptool,不必自寫 3.5" GCR 解碼器)
WOZ = Applesauce 的 flux/bit-stream,3.5" 用 GCR 6-and-2。**MAME `floptool` 內建編解碼**:
```bash
floptool flopdir  woz prodos "<game.woz>"                       # 列 ProDOS 目錄
floptool flopread woz prodos "<game.woz>" <PATH> <out>          # 抽檔
# 注意:抽帶 resource fork 的檔 → data fork 寫 <out>;resource fork 寫 ._<out>(AppleDouble)
```
> Ultima I IIgs woz:volume `ULTIMA.I`,只有 `FINDER.DATA` + `ULTIMAI`(data 90K + rsrc fork 580K)。
> resource fork 解析見 `e2-apple-iigs.md`(IIgs Resource Manager;type 0x8001 rIcon、0x8024 音效、0x0001 壓縮圖)。

## 3. ★ 遊戲碟非自我開機 → 需 GS/OS 系統碟
- 直接 `-flop3 game.woz` 開機 → IIgs ROM 印 **「UNABLE TO LOAD PRODOS」**(藍底)。
  因為遊戲碟是 **GS/OS 應用程式碟**(無 `PRODOS`/`System` 系統檔),不能單碟開機。
- 解法:**GS/OS System 6.0.x 開機碟**。本機/已知源找不到 → **WebSearch**(別盲猜 URL,反爬蟲會給 418/503):
  archive.org `IIGSSystem6.0.22mgDisks` → `IIGS System 6.0.2 2mg disks.zip`(8 張碟,**Disk 2 = 可開機 System disk**)。
- 開機:`-flop3 "…Disk 2 System disk.2mg" -flop4 "<game.woz>"` → GS/OS 6.0.2 開到 **Finder 桌面**
  (System Disk + 遊戲碟兩圖示)。⚠ GS/OS 開機慢:**~150–270s emulated,變異大**。

## 4. ★ Lua autoboot 自動化(鍵盤 / 滑鼠 / dump)
`-autoboot_script foo.lua`(配 `-autoboot_delay`)。核心 API(MAME 0.264):
```lua
local mac=manager.machine
emu.register_frame_done(function() ... end)        -- 每 frame callback(@60fps)
local port=mac.ioport.ports[":macadb:KEY2"]        -- ioport tag
local field=port.fields["Return"]                  -- field 名(見下)
field:set_value(1)  -- 按下;set_value(0) 放開(persist 到下次改;用 edge-detect 避免每 frame 重設)
mac.video:snapshot()                               -- 存 PNG(需 video,非 -video none)
-- ⚠ mac:schedule_exit() 在這版不存在 → 用 -seconds_to_run N 控總長;mac:save() 存檔狀態不穩(沒成功)
```
**apple2gs ADB ioport map**(`mame.ioport.ports` 掃出來):
| port | 內容 |
|---|---|
| `:macadb:KEY0` | a b h … 字母 |
| `:macadb:KEY1` | o y 0-9 … |
| `:macadb:KEY2` | u k n **Return** … |
| `:macadb:KEY3` | **Command / Open Apple**、**Space**、**Esc**、**Up/Down/Left/Right Arrow** |
| `:macadb:KEY4` | Keypad Enter |
| `:macadb:MOUSE0/1/2` | Mouse Button / X / Y(相對) |

**★ ADB 鍵盤會緩衝**:在 Finder 還沒就緒前送的 'u'+Cmd+O 仍會被佇列、待 Finder 起來後處理 →
不必精準對時(但 Return 對「之後才出現的對話框」無效,要按在對話框出現之後)。

## 5. SHR 螢幕 dump + 渲染
SHR framebuffer = **bank $E1 的 `$2000–$9FFF`**(maincpu program space `0xE12000–0xE19FFF`,32KB):
```lua
local sp=mac.devices[":maincpu"].spaces["program"]
local f=io.open("/work/.../shr.bin","wb")
for a=0xE12000,0xE19FFF do f:write(string.char(sp:read_u8(a))) end; f:close()
```
**SHR 格式**(`tools/re/iigs/render_shr.py`):
- `$0000–$7CFF` 像素:200 行 × 160 B/line,**4bpp chunky**(高 nibble = 左像素)。
- `$7D00–$7DC7` SCB(每行一 byte):低 nibble = 該行 palette index(0-15),bit7 = 640 模式。
- `$7E00–$7FFF` 16 個 palette × 32 B:每色 12-bit `$0RGB` word(LE)→ R=(w>>8)&F, G=(w>>4)&F, B=w&F,各 ×17 到 8-bit。

## 6. ★ 效能:`-video none` vs xvfb
- **`-video none`**:無需 X,跑 ~835%(310s emulated ≈ 40-90s wall)。dump SHR 記憶體**不需 video** → 自己渲染。**迭代首選**。
- **xvfb + `mac.video:snapshot()`**:要看「實際畫面」(含 text/hires 模式、game native PNG)時用;慢很多。
  `u1-mame` image 要含 `xvfb x11-utils imagemagick`。
- 教訓:除錯導航時 dump SHR 記憶體 + 自己渲染,比 xvfb snapshot 快 ~8×。

## 7. 完整遊戲導航流程(Ultima I IIgs,已驗證可達角色創建)
鍵盤全自動(`tools/re/iigs/mame_nav_chargen.lua`):
1. GS/OS 開機到 Finder → type-select `u`(選遊戲碟)+ **Cmd+O ×2**(開碟視窗、啟動 ULTIMAI)。
2. ULTIMAI:Tool035 MidiSynth 警告 → **Return/Space/Enter** 之一可關。
3. 主選單「A) Generate new / B) Continue previous save」→ 按 **A**。
4. Character Generation:**Right=加屬性、Down=換屬性**(單屬性上限~25,**需分散** 30 點),分配完 **Space**。
5. race(a-d)/sex(a-b)/class(a-d) 各按字母;`Enter thy name` 打字 + Return。
6. 「Save this character? (Y-N)」→ **Y** → GS/OS「Save thy game as…」對話框。

## 8. ⛔ 牆:GS/OS 標準檔案對話框只認滑鼠
- 存檔 SFPutFile / 讀檔 SFGetFile(「Which game my Lord?」,主選單 B):**鍵盤 Down/Return/type-select 都無法選檔開啟** → 退回主選單。
- 滑鼠(ADB 相對,`set_value(delta)` 每 frame 累加):**pin 到角落已驗證**(`MOUSE_X=-30` 連 60 frame → 游標到 (0,0))。
  但移到檔案/按鈕要校準 delta→pixel(640 模式 QuickDraw 座標),且存+讀兩對話框各需校準 → 多輪 ~90s 迭代,**未攻克**。
- 替代:① 找現成存檔檔放碟上;② 改造碟設 ULTIMAI auto-launch 免 Finder;③ **直接看 §9**。

## 9. ★★ 最快的路:原生解析度實機截圖直接切 tile(本案最終解)
卡在 §8 滑鼠 + 自訂壓縮兩頭難時,**換角度**:hg101 的 Apple IIgs 截圖是 **320×200 原生 SHR、PNG 無損**,
每 tile 正好 16×16 → 直接切(`tools/build_iigs_pack.py`):
- overworld tile 網格相位 **offset (8,8)**(玩家居中推得);dedup 切 unique tile。
- 截圖 07/12 → 水/草/森林/山/城堡/城鎮/玩家/馬 共 **8 個真實 IIgs tile**;未涵蓋槽以 EGA 補成 hybrid。
- player/horse 截圖含草地背景 → 去綠轉透明。**game tester 驗證 overworld 渲染正常**。
> 教訓(已記 memory `retro-native-screenshot-tile-rip`):素材抽取卡格式/模擬器時,**先找原生解析度實機截圖**,
> 往往比硬解格式或自動化模擬器快幾個數量級。同 retro「反編當 oracle 不照抄」「先建可驗證訊號」。

## 10. 踩雷清單
- [雷] `mame` 套件存在但 `apt-cache policy` 說無候選 → 先 `apt-get update`。binary 在 `/usr/games/mame` 非 PATH。
- [雷] 直接開遊戲 woz → 「UNABLE TO LOAD PRODOS」:遊戲碟非自我開機,要 GS/OS 系統碟(flop3)。
- [雷] GS/OS 開機慢且時間變異大(150-270s)→ 絕對 frame 對時不可靠;靠 ADB 鍵盤緩衝 + 把動作排在夠晚。
- [雷] `mac:schedule_exit()` 不存在(這版)→ 用 `-seconds_to_run`。`mac:save()` 存狀態沒成功。
- [雷] Return 對「之後才出現」的對話框無效(時序);GS/OS 檔案對話框根本不認鍵盤,只認滑鼠。
- [雷] dump SHR 用 `-video none` 快 8×;要看真實畫面才用 xvfb snapshot。
- [雷] `_pyc`/暫存別入庫;copyright(ROM/woz/2mg/dump)放 `re_work/`(gitignore)。

## 11. 環境 / 工具一覽
- `docker/Dockerfile.mame`(u1-mame:`u1-a2` + mame + xvfb;u1-a2 = u1-re + mame-tools/floptool)。
- `docker/Dockerfile.a2`(floptool)、`docker/Dockerfile.z80`(z80dasm,另用)。
- `tools/re/iigs/`:`extract_woz.py`(AppleDouble+resource 解析)、`render_shr.py`(SHR→PNG)、
  `mame_dump.lua`(SHR dump)、`mame_nav_chargen.lua`(開機+角色創建導航)。
- `tools/build_iigs_pack.py`(實機截圖切 tile → 832×16 PNG)。
- copyright:`re_work/iigs/bios/`(ROM)、`re_work/iigs/sys/`(GS/OS 2mg)、`re_work/Ultima I IIgs.woz`。
