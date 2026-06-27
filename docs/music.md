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

## 其餘平台(待渲染)

chiptune 平台(MSX PSG / Atari POKEY / PC-98 FM / IIgs synthLAB)無法直接轉檔,
需在各自模擬器內播放遊戲、錄下音訊軌再轉 ogg(emulator audio capture)。屬獨立工項,逐一補上。
