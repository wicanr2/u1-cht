# CONTEXT — Ultima I 中文化專案語彙與背景

> 進本 repo 工作前先讀。術語表(glossary)隨新名詞維護。

## 專案一句話
把 `matiaslaino/open_ultima`(C++17 / SDL2 的 Ultima I 開源重製,**開發極早期**)
在 Linux 上 build 起來,並做**繁體中文化**(CJK TTF 字庫 + UTF-8 字串管線)。

## 與範本專案的關係
| 範本 | 對本專案的價值 |
|---|---|
| `../u3-cht` | 最近完成;PLAN/docker/CONTEXT/字型 pipeline 格式直接沿用 |
| `../u4-cht` | i18n 字串表、按鍵與 UI 慣例參考 |
| `../u7-cht` | FM Towns 素材來源、美術參考 |

## 上游現況(重要:範圍認知)
open_ultima 自述「very very early」。**已實作**:世界地圖移動、基礎地牢探索、基礎地牢戰鬥;
**城鎮/城堡為空殼**。⇒ 第一階段「中文化」對象 = 現有 UI/指令/戰鬥訊息字串(量小,多為原始碼硬編碼)
+ 換 CJK 字型 + Linux build。**U1 完整劇情文字(NPC 對話、招牌、開場)上游尚未實作**,屬後續隨功能補。

## 術語表(glossary)
| 英文 | 中文 | 說明 |
|---|---|---|
| Overworld | 世界地圖 | 64×64 大地圖(Sosaria) |
| Town / Castle | 城鎮 / 城堡 | 上游目前空殼 |
| Dungeon | 地牢 | 第一人稱 3D 線框視角 |
| Tile | 地圖磚 | 16×16 px |
| EGA / CGA | EGA / CGA | 兩套 PC 顯示色盤(原始資料含 `EGATILES.BIN`/`CGATILES.BIN`) |
| Klimb | 攀爬 | U1 上下樓梯指令(經典拼法) |
| Ztats | 屬性 | 角色數值畫面 |
| Lord British | 不列顛王 | 官方精訊譯名(creature_bilingual);全處統一 |
| Mondain | 蒙登 | 大反派 |
| Sosaria | 索薩利亞 | U1 世界 |
| Dagger / Mace / Bow | 短劍 / 釘頭鎚 / 弓 | 武器,對齊精訊 config_bilingual |
| Chain / Plate | 鎖子甲 / 鎧甲 | 防具,對齊精訊 |

> **譯名權威來源(聖者之書)**:`../u4-cht/manuel/`(精訊《創世紀聖者之書特別版》PDF + `sage_pages/`)、
> `../u4-cht/dumps/*_bilingual.json`(官方中文版抽出的怪物/裝備/城堡雙語對照)。
> 新增名詞先查此處;系列共通名(怪物/裝備/Lord British)一律沿用官方譯名。

## 技術關鍵事實(供快速定位)
- 邏輯解析度 `320×200`(`src/Constants.h`),整數倍放大輸出。
- 文字渲染**單一出口**:`LTexture::loadFromRenderedText` → 目前 `TTF_RenderText_Solid` + 8px `PC_Senior_Regular.ttf`。
  中文化 hook 點即此處:改 `TTF_RenderUTF8_*` + CJK TTF。
- 設定/資料路徑集中於 `src/Configuration.h`,讀 `config.json`(需自備原始遊戲 `*.BIN`)。
- 上游用 `taoJSON` + MSVC `throw exception("...")`(非可攜)→ Linux 需改 `nlohmann/json` + `std::runtime_error`。

## 硬規則
- 編譯一律走 docker(見 `docker/Dockerfile`)。
- 原始遊戲資料檔(版權)不入庫(`.gitignore` 已排除 `*.BIN` / `gamedata/`)。
- 字型不用 cubic,用優質系統中文 TTF 重建字庫(專案指示)。
- 每完成一段落 commit + push 到 `github.com/wicanr2/u1-cht`。
