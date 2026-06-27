# 各版本背景音樂(音樂跟隨平台)

切換 tileset(`F1` / `PageDown`)時,背景音樂會**一併換成該平台原版的 BGM** —— 看哪一版的畫面,
就聽哪一版的音樂。實作見 `main.cpp`(`pngPacks[].music` + `applyMusic()`)與 `src/common/Audio.cpp`。

## 版權與檔案放置

各平台原版 BGM 為**版權內容,不隨 repo 散布**(同遊戲資料)。引擎按下表路徑載入;
**缺檔則自動退回占位曲 `theme.ogg`**,不中斷遊戲。使用者自備、放入 `assets/music/`:

| 平台 | 檔案 | 來源 / 取得方式 | 狀態 |
|---|---|---|---|
| FM Towns | `assets/music/fmtowns.ogg` | Trilogy CD 的 `MAP.EUP`(EUPHONY FM 音樂)→ 渲染成 ogg(見下) | ✅ 可用 |
| MSX | `assets/music/msx.ogg` | `.dsk` PSG 音樂 → openMSX 錄製 | ⬜ 待渲染 |
| Apple IIgs | `assets/music/iigs.ogg` | woz 的 synthLAB 樂曲 → MAME 錄製 | ⬜ 待渲染 |
| PC-98 | `assets/music/pc98.ogg` | `.fdi` 的 `SCORE.DAT`(YM2608 FM)→ PC-98 模擬器錄製 | ⬜ 待渲染 |
| EGA / CGA / VGA(DOS) | `theme.ogg`(占位) | DOS 原版僅 PC speaker,無獨立 BGM | — 占位 |

> `assets/music/*.ogg` 已 gitignore(僅占位 `theme.ogg` 入庫)。

## FM Towns 音樂取得(已有工具)

FM Towns 版 BGM 是 **EUPHONY(.EUP)FM 音樂**,配 `ULTIMA.FMB` 音色庫。Trilogy CD 內檔案:
`MAP.EUP`(overworld)、`TOWN.EUP`、`DUNGEON.EUP`、`OSIRO.EUP`(城堡)等。
渲染成 ogg 後放成 `assets/music/fmtowns.ogg`(目前用 `MAP.EUP` overworld 主題,53 秒循環)。

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
