# Ultima I: First Age of Darkness 中文化 — 執行計畫 (PLAN)

> 本檔由 `CLAUDE.md` 潤飾展開為可執行工程計畫。
> 立案日:2026-06-26 ・ 維護:L.CY (anr2) + Claude
> 基礎:`matiaslaino/open_ultima`(C++17 / SDL2,MIT)。背景與語彙見 `CONTEXT.md`。

---

## 0. TL;DR

把 **`open_ultima`(已是 C++17 + SDL2 的 Ultima I 開源重製)** 在 **Linux(Docker)** build 起來,
換上 **CJK TTF 字庫 + UTF-8 字串管線**,做**繁體中文化**。

* 與 U3 不同:**不需移植 Mac 平台層**——上游本來就 SDL2。主要工是 ① Linux 可攜性修正 ② 中文渲染 ③ 字串抽取翻譯。
* ⚠️ 上游**開發極早期**:世界地圖/基礎地牢可動,城鎮城堡空殼。第一階段中文化對象 = 現有 UI/指令/戰鬥訊息(量小)。完整劇情文字隨上游功能補。
* 全程 **Docker build**;**截圖**當決定性 pass/fail loop;**game tester 背景驗證**。
* **每段落 commit + push** 到 `github.com/wicanr2/u1-cht`。

---

## 1. 目標與範圍

### 1.1 目標
Linux 上可執行、現有畫面文字全繁中化的 open_ultima,操作與訊息對齊 U1 語意。

### 1.2 成功標準(可驗證)
1. Docker 內 CMake 產出 Linux native ELF,讀 `config.json` + 原始 `*.BIN` 啟動不崩潰。
2. 世界地圖移動、地牢探索/戰鬥三類畫面可繪製與切換。
3. 現有指令回饋、戰鬥訊息、方向、敵人名稱、地牢層數以**中文**顯示,字型清晰、不溢框。
4. ESC=返回 / F10=離開(確認+自動存檔)語意一致(見 `docs/keybindings.md`)。
5. game tester 背景跑完最小腳本(進世界→進地牢→打一場→離開)無 regression。

### 1.3 範圍外(初期)
* macOS / Windows 原生包(SDL2 跨平台後補,先 Linux)。
* 重繪美術 / 高解析 tileset(沿用原始 EGA/CGA 解碼)。
* U1 完整劇情文字(上游未實作的城鎮 NPC、開場、招牌)——隨上游功能補。
* 原始遊戲資料散布(版權;引擎與資料分離,使用者自備 `*.BIN`)。

---

## 2. Repo 佈局

```
/home/anr2/u3-cht/                 ← 工作區(放參考素材)
├── u3-cht/ u4-cht/ u7-cht/        ← 範本(格式/字型/素材參考)
└── u1-cht/                        ← ★ 本專案 git repo (remote: wicanr2/u1-cht) ★
    ├── CLAUDE.md  PLAN.md  CONTEXT.md  README.md
    ├── main.cpp  CMakeLists.txt   ← 由上游改寫(Linux/pkg-config)
    ├── src/                       ← open_ultima 源碼 in-place fork(就地中文化修改)
    ├── resources/                 ← 字型(含待加入的 CJK TTF)
    ├── assets/
    │   ├── fonts/                 ← 重建之中文 TTF/字庫
    │   └── strings/zh-Hant/       ← 翻譯字串表(ui.json…)
    ├── i18n/                      ← 字串抽取/工具產物
    ├── docker/Dockerfile          ← SDL2 + SDL_image/ttf/gfx + nlohmann-json + noto-cjk
    ├── tools/                     ← 字串抽取、截圖比對腳本
    ├── tests/snapshots/           ← game tester 腳本 + 畫面基準
    ├── docs/                      ← keybindings、ADR、移植筆記
    ├── config.json.example        ← 設定範本(實際 config.json 不入庫)
    └── open_ultima/               ← 上游唯讀 clone(.gitignore;diff/oracle 用)
```

**決策**:`src/` 直接 fork 上游就地修改(本專案是「在地化既有開源專案」,非從零移植);
`open_ultima/` 保留原始 clone 作 diff/oracle,gitignore 不入庫。

---

## 3. 技術堆疊與上游差異

| 項目 | 上游 (open_ultima) | 本專案 (Linux 中文化) |
|---|---|---|
| 建置 | vcpkg + MSVC + CMake | Docker + apt + pkg-config + CMake |
| JSON | taoJSON | **nlohmann/json**(apt 可取) |
| 例外 | `throw exception("msg")`(MSVC 擴充) | `std::runtime_error`(可攜) |
| 文字渲染 | `TTF_RenderText_Solid` + 8px ASCII TTF | `TTF_RenderUTF8_Blended` + CJK TTF |
| 字串 | 硬編碼英文 | i18n 查表(`assets/strings/zh-Hant/*.json`) |
| 繪圖/事件/音 | SDL2(不變) | SDL2(沿用) |

---

## 4. 工作分解(分階段;每階段一段落 → push)

### Phase 0 — 鳥巢框架(本次完成 ✅)
- 目錄結構、Docker、CMakeLists(Linux)、CONTEXT/PLAN/keybindings、i18n 骨架、.gitignore。

### Phase 1 — Linux 可編譯垂直切片(最高優先,決定性 loop)
1. `Configuration.*`:taoJSON → nlohmann/json。
2. 全域 `throw exception("...")`(2 處)→ `std::runtime_error`;補 `<stdexcept>`。
3. include 風格 / `<SDL.h>` 路徑、MSVC-only 慣用法逐一掃除直到 docker build 通過。
4. **驗收**:Docker 內 `cmake && make` 產出 `u1_cht` ELF。
> 需使用者提供原始 U1 PC 資料檔(`EGATILES.BIN`/`MAP.BIN`/… 來自 GOG)放 `gamedata/` 才能實跑;
> 編譯本身不需資料檔。

### Phase 2 — 中文渲染管線
1. 選一套可商用/系統 CJK TTF(noto-cjk 或指定字型),放 `assets/fonts/`,重建字庫(不用 cubic)。
2. `Fonts::init`:載入 CJK TTF(字級放大,16/24px;搭配 320×200→整數倍縮放維持清晰)。
3. `LTexture::loadFromRenderedText`:`TTF_RenderText_Solid` → `TTF_RenderUTF8_Blended`。
4. CJK 換行/寬度:`CommandDisplay`(`MAX_LINE_CHARS=29`)改全形寬度感知,不溢框。
5. **驗收**:截圖顯示一句中文戰鬥訊息,字清晰、對齊正確。

### Phase 3 — 字串抽取與翻譯
1. `tools/` 寫抽取腳本掃 `src/` 硬編碼英文 → 補齊 `assets/strings/zh-Hant/ui.json`。
2. 加極簡 i18n 查表層(`I18n::t("key", {name})`),把 `CommandDisplay::writeLn(...)` 等改走 key。
3. 翻譯方向/指令/敵人/層數/戰鬥訊息;`{name}` 佔位符與全形標點校訂。
4. **驗收**:現有畫面所有可見英文字串改中文,game tester 腳本逐畫面截圖比對。

### Phase 3.5 — 可替換素材包(tileset / sprite,對應使用者需求)
> 詳見 `docs/adr/0001-pluggable-asset-packs.md`。
1. 第一步:tileset 變體改 **config 驅動**(`ega`/`cga`/`tandy`,皆現成 BIN),取代寫死 `usingEga=true`。
2. 抽 `AssetPack` 介面:來源 = 原始 BIN 解碼 / PNG sprite sheet 兩種。
3. 非 DOS 版美術(Apple/FMTowns/MSX2…)從 `reference/hg101/` 截圖拆 sprite → 統一 PNG 切版規格。
4. **驗收**:同一存檔切 `tileset` 設定可換不同版本外觀。

### Phase 4 — 操作介面與系統鍵(對齊範本)
1. 事件迴圈收斂 ESC(返回)/ F10|Ctrl+Q(離開→確認+自動存檔),見 `docs/keybindings.md`。
2. 自繪中文 quit 確認對話框。
3. **驗收**:任一畫面按 ESC 不離開、F10 跳確認、確認後存檔退出。

### Phase 5 — 打包與回歸
1. dist 腳本(Linux;AppImage 可選)。
2. game tester 完整腳本納入 `tests/`,當回歸基準。
3. README 完稿(安裝、自備資料檔、操作、已知限制)。

---

## 5. 驗證策略
- **Docker first**:所有 build/test 於 `docker/Dockerfile` 容器內,xvfb 跑無頭截圖。
- **決定性 pass/fail loop**:逐畫面截圖(`tests/snapshots/`)比對基準。
- **game tester**:背景啟動,跑「進世界→進地牢→戰鬥→離開」最小腳本驗收,不阻塞對話。
- **oracle**:行為/字義對不上時,對照 `open_ultima/` 原始上游與 U1 原版資料格式文件。

---

## 6. 風險 / 待確認
1. **🟠 原始遊戲資料檔**:open_ultima 需 GOG 版 `*.BIN`。需使用者提供放 `gamedata/`(編譯不需,實跑需)。FM Towns 素材為美術參考,非此引擎直接讀取格式。
2. **🟠 上游極早期**:城鎮/城堡空殼,完整 U1 文字尚無處可譯;第一階段交付聚焦現有畫面,範圍誠實揭露。
3. **🟡 CJK 字級 vs 320×200**:8px → 中文需放大字級 + 整數倍縮放;`MAX_LINE_CHARS` 全形換行需改。
4. **🟡 MSVC→GCC 可攜性**:`exception("...")`、潛在 `<windows.h>` 慣用法、型別寬度需逐一掃。
5. **🟡 i18n 介面**:上游字串散在原始碼,需抽取 + 查表層;`{name}` 等動態片段語序中文化。
6. **🟡 字型授權**:採可商用/系統 TTF(noto-cjk);引擎與資料分離利於日後散布。

---

## 7. 下一步(等使用者確認後執行)
1. **Phase 1 垂直切片**:Docker 環境 build 通過(taoJSON→nlohmann、例外可攜化)。— 風險最高、優先打掉。
2. 請使用者提供 GOG 版 U1 原始資料檔(供實跑),或確認先只做「可編譯 + 中文渲染截圖」驗證。
3. 確認 CJK 字型選擇(noto-cjk 預設,或指定字型)。
