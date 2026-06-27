# 各版本背景音樂(音樂跟隨平台)

切換 tileset(`F1` / `PageDown`)時,背景音樂會**一併換成該平台原版的 BGM** —— 看哪一版的畫面,
就聽哪一版的音樂。實作見 `main.cpp`(`pngPacks[].music` + `applyMusic()`)與 `src/common/Audio.cpp`。

## 版權與檔案放置

各平台原版 BGM 為**版權內容,不隨 repo 散布**(同遊戲資料)。引擎按下表路徑載入;
**缺檔則自動退回占位曲 `theme.ogg`**,不中斷遊戲。使用者自備、放入 `assets/music/`:

| 平台 | 檔案 | 來源 / 轉換方式 | 狀態 |
|---|---|---|---|
| **PC-98** | `assets/music/pc98.ogg` | `.fdi` 的 `SCORE.DAT`+`VOICE.DAT`(YM2608 FM)→ **自包含 2-op FM 合成器原生 render**(`render_pc98_music.py`,不跑模擬器) | ✅ **可用**(52.9s,RMS 9462) |
| **FM Towns** | `assets/music/fmtowns.ogg` | Trilogy CD 的 `MAP.EUP`(EUPHONY)→ **自寫 EUP→2-op FM 渲染**(`render_eup_music.py`) | ✅ **可用**(49.9s,RMS 3624;原 u7-cht ogg 為靜音,已重做) |
| **MSX** | `assets/music/msx.ogg` | `.dsk` 的 `ULT*.MCP`(Pony Canyon 音樂格式)→ **自寫 MCP→FM 渲染**(`render_mcp_music.py`) | ✅ **可用**(`ULT1`="LORD BRITISH'S THEME" overworld,41.4s RMS 5952) |
| Apple IIgs | `assets/music/iigs.ogg` | woz 的 synthLAB / Ensoniq 5503 取樣合成 | ⬜ 待解格式 |
| Atari | `assets/music/atari.ogg` | POKEY,OUTMOVE/SPAMOVE 內 | ⬜ 待定位 |
| EGA / CGA / VGA(DOS) | `theme.ogg`(占位) | DOS 原版僅 PC speaker,無獨立 BGM | — 占位 |

> `assets/music/*.ogg` 與 `assets/music/**/*.ogg` 已 gitignore(僅占位 `theme.ogg` 入庫)。

## ★ PC-98 音樂(已完成,native FM 合成)

`SCORE.DAT` 序列格式 RE(`render_pc98_music.py`):
- 開頭 16-bit LE offset 表 → **10 首歌**。每首開頭 **6 個 channel 指標**(YM2608 6 FM 聲道)。
- channel 資料 = 控制碼(byte ≥ 0xe0)+ **(note, dur)** 兩 byte 事件(note=MIDI 音高、dur=tick 24 倍數)。
- 控制碼參數長度(暴力求解):`e0`/`e2`/`f4`/`fa`=1 參數、`fb`/`fc`=0 參數、`fe`=結束。
  (這是破解關鍵:參數長度不一造成對齊漂移,修正後 note 全落合理音域。)
- **發聲 = FM**:用自包含 2-op FM(modulator→carrier)+ ADSR 包絡渲染,保留 chiptune FM 質感,**不跑模擬器**。
- 10 首全渲(`assets/music/pc98/song0-9.ogg`);overworld 暫用 **song1**(三聲部、旋律完整,piano-roll 驗證為連貫音樂)。
  ⚠ 哪一首是 overworld 需聽感確認(FM Towns 參考曲為靜音,無法自動比對)。

## ★ MSX 音樂(已完成,native MCP→FM)

MSX 版(Pony Canyon 1986)BGM 在 `.dsk` 的 **`ULT*.MCP`** 檔(9 首,各帶標題,如 `ULT1`="LORD BRITISH'S THEME")。
MCP 與 PC-98 SCORE / FM Towns EUP **同作曲團隊**(共用 `e0`/`e2`/`fa` 控制碼)。`render_mcp_music.py`:
- `fe fe fe fe` = 音軌分隔;**4 byte/事件**:控制碼(byte0 ≥ 0xe0 或 0xfd)或音符 `[note, gate, step, vel]`。
- step = delta tick;3 軌(MELODY / BASS / CHORD)。沿用 2-op FM 合成。overworld = `ULT1`。

## ★ FM Towns 音樂(已完成,native EUP→FM 渲染)

FM Towns 版 BGM 是 **EUPHONY(.EUP)** 格式(`MAP.EUP` overworld、`TOWN`/`DUNGEON`/`OSIRO` 等)。
u7-cht 既有 `sound_extract/eup_*.ogg` 經驗證為**靜音**(原轉檔失敗),已用自寫渲染重做。

`render_eup_music.py` 解 EUP:
- 檔頭(標題 + 固定音名表)後有 `"EUPHONY "` 簽章 → 其後 **6 byte/事件**。
- 事件:`byte0`=status(0x9n note-on / 0x8n note-off,ch=status&0xF)、`byte1`=**step delta tick**
  (全曲累積,sum≈4930 對上 ~53s)、`byte4`=note、`byte5`=velocity;note-off 結束該聲道音符。
- 用同 PC-98 的 2-op FM 合成渲染,**不跑模擬器**。map/town/dungeon/castle 全渲於 `assets/music/fmtowns/`。

## 其餘平台:逐平台找檔 → 確認格式 → 原生轉回(不跑模擬器)

方法依 L.CY 指示:不錄模擬器,**找到音樂資料檔、確認格式、理解發聲機制、原生 render 回 ogg**。
各平台格式調查(進行中):

| 平台 | 音樂檔 | 格式 / 發聲機制 | 轉換路徑 | 狀態 |
|---|---|---|---|---|
| **PC-98** | `SCORE.DAT`(5433B)+ `VOICE.DAT`(1024B) | **YM2608(OPNA)FM**。SCORE.DAT 開頭是 ~10 首歌的 16-bit offset 表(`$0028 $00e2 $02e0 $052e…`)+ 序列資料;VOICE.DAT = FM 音色(operator 參數) | RE 序列格式 → 轉 VGM / 用 OPNA 合成器 render | 🟡 檔+格式已定位 |
| **Apple IIgs** | woz resource(`id12`/`id13` ~3KB 等)+ Ensoniq 取樣 | **synthLAB / Ensoniq 5503 DOC**(32 振盪器取樣合成) | RE 樂曲序列 + 取樣 → 用 5503 合成器 render | 🟡 resource 已抽,格式待解 |
| **MSX** | `.dsk` 內音樂資料 / OUT.COM | **PSG(AY-3-8910)** 或 SCC | 定位序列 → PSG 暫存器 log → render | ⬜ 待定位檔案 |
| **Atari** | `OUTMOVE`/`SPAMOVE.bin` 內 | **POKEY** | 反組譯找音樂常式 + 序列 → POKEY render | ⬜ 待定位 |

> 每個 chiptune 格式都是一份獨立 RE(序列格式 + 音色 + 合成器),工程量與 tileset 相當。逐平台推進。
> FM Towns 之所以最快,是因為 EUPHONY(.EUP)是有現成播放器/轉檔工具的成熟格式。
