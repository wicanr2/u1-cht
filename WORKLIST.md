# Ultima I 繁中化 — 工作清單(WORKLIST)

> 進度追蹤。✅=完成並驗證 / 🔜=排程中 / ⬜=待辦 / 🧱=架構牆(大工程)。
> 每完成一項:tester 驗證 → commit → push → 勾選。更新日:2026-06-26。

## A. 已完成 ✅

- [x] Linux 可編譯可執行(taoJSON→nlohmann、MSVC 例外可攜化)
- [x] 全畫面繁中翻譯(世界/地牢/狀態列/戰鬥/敵人/方向)+ 視窗標題
- [x] 拉高內部畫布 320→640(底圖 crisp,中文不糊)
- [x] 宋體(Noto Serif CJK)抗鋸齒 + 1280×800 乾淨 2× 縮放
- [x] F10/Ctrl+Q 離開確認對話框 + ESC 取消(離開鐵則)
- [x] tileset 熱鍵 F1/PageDown(EGA/CGA)+ 螢幕中文提示
- [x] 切 tileset NPC 消失/變色修法
- [x] 音樂系統(SDL_mixer)+ M 鍵切換(占位音樂)
- [x] 回合制生怪 + `spawn_pct`/`speed_pct`(config 可調,參考 u2-cht)
- [x] 地面怪相鄰反擊(忠於原版:不移動)
- [x] 地牢怪 beeline 移動 + 對角攻擊(忠於原版)
- [x] 原版 U1 AI 考據 + 6502 判讀方法論(逆向 1983 Atari)
- [x] game tester harness(xvfb+xdotool 驅動正常玩家路徑)

## B. 排程中(本批要做)🔜

- [x] **F6 設定選單**:遊戲內即時調 `speed_pct`/`spawn_pct`(目前只能改 config.json)
  - 自繪中文 modal、↑↓選列、←→±5%、F6/ESC 關閉;沿用離開鐵則的 modal 風格
- [x] **食物 / 飢餓 tick**:每時間 tick 食物 −1;食物=0 時每回合扣 HP(參考 u2-cht 機制)
  - 掛在 `OverworldScreen::onStep` 與 `DungeonScreen` 的時間 tick;狀態列食物已顯示
- [~] **跨版本素材包(AssetPack,ADR 0001)**:可換不同平台外觀
  - [x] 第一步:tileset 變體 config 驅動(`tileset: ega/cga`)+ F1/PageDown 循環
  - [ ] 逐平台拆解 + 格式修正 → 見 **§E**

## E. 跨平台素材包 — 逐平台拆解(B3 進階,本批)

> 目標:每平台抽 **overworld tileset(52 格)+ 主要 sprite(主角/怪物)** → 統一 PNG sprite sheet
> (對齊 engine tile index)→ open_ultima 用 PNG AssetPack 載入,可 config/熱鍵切換不同版本外觀。
> DOS tile 格式已知(EGA BGRI planar 4bpp、16×16、128B/格、52 格)。素材見 `docs/materials.md`。

### E0. 基礎建設(前置,先做)
- [x] **PNG AssetPack 載入器**:open_ultima 加「PNG sprite sheet → overworld tileset」載入路徑
- [x] **EGA→PNG 匯出工具**:把現有 DOS tileset 解碼匯出 PNG(參考包 + 驗證 PNG pipeline)

### E1–E5. 逐平台(一個一個做)
| # | 平台 | 素材 | 格式 / 可複用經驗 | 狀態 |
|---|---|---|---|---|
| E1 | **FM Towns** | Trilogy CD(GRAPH 已抽於 u7-cht) | ✅ 地形渲染成功(綠草+藍水+森林);8色palette(偶數index)+ chunkyrev 解碼 + 自動分類 + 32→16。物件/怪物 sprite 精修待續 | ✅ 地形完成 |
| E2 | **Apple IIgs** | woz(1994) | 🧱 **woz 解碼牆**:需自寫 3.5" GCR 解碼器(見 docs/re/e2-apple-iigs.md) | 🧱 |
| E3 | **MSX** | `org_game/msx/…Pony Canyon….dsk` | MSX 磁碟 + pattern/VRAM 格式 | ⬜ |
| E4 | **PC-98** | `org_game/msx/【PC98】….fdi` | PC-98 FDI + planar 圖格式 | ⬜ |
| E5 | **Atari** | 1983 ATR(已抽檔)| tile 在 `SET1-5`/`MASTER`?待找;6502 已反組譯 | ⬜ |

### E1 FM Towns 續作(已解碼,剩 palette+對應)
- [ ] U1 專屬 16 色 palette(RE U1 FM Towns 執行檔 / 對 hg101 FMTOWNS-06 實機採樣)— header 無內嵌 ColorMap
- [ ] 52 格對應(UT1MAP 地形 + UT1TILE1 物件 + UT1TILE0 怪物 → engine 槽)
- [ ] 32→16 downscale 拼 832×16 → 載入校驗(對實機)

### ★ 素材格式需修正 / 校準(實作中發現,逐項修)
- [ ] **Tandy(T1K)**:`T1KTILES.BIN` 與 EGA **同大小(6656B)但格式不同** → EGA decoder 解出亂碼,
      **需專屬 T1K decoder**(Tandy 1000 16 色記憶體佈局)。記:同大小≠同格式。
- [ ] **palette / index 對應校準**:各平台 palette 與 engine tile index 須以**實機截圖**(`reference/hg101/`)校準,
      否則色彩/對應錯位(u2-cht FM Towns 經驗:sprite 只用偶數 nibble、palette 對齊實機截圖)。
- [ ] **CGA palette 確認**:open_ultima CGA 為青/洋紅標準盤,確認與原版一致(或提供盤切換)。

> 附帶可複用:FM Towns CD 音樂 `extract_fmtowns_cdda.py` → 對應「FM Towns 真音樂」backlog。
> 每平台流程:抽 → 轉 PNG(對齊 index)→ 載入驗證(截圖)→ commit。

## C. 待辦(backlog)⬜

- [ ] **存檔系統**:目前無存檔;F10「Y 離開」前的 autosave 仍是 TODO
- [ ] i18n 查表層:翻譯目前內聯,遷移到 `assets/strings/zh-Hant/*.json`
- [ ] `CommandDisplay` 全形寬度感知換行(目前 UTF-8 邊界安全但非全形對齊)
- [ ] FM Towns 真音樂替換占位 ogg(從 Trilogy CD 映像抽 CD-DA)
- [ ] Town / Castle 內容(open_ultima 上游為空殼)
- [ ] 戰鬥深度:武器 / 法術 / 物品欄(上游極早期)
- [ ] 打包(AppImage / Windows)— **使用者指示不急著打包**

## D. 架構牆 / 長期 🧱

- [ ] 逆向常駐引擎逐行確認怪物移動 6502 routine(模擬鏈載入→重定位→$3000 引擎)
  - 續追路徑見 `docs/re/README.md`;目前 AI 行為已足以忠實實作(B/C 不被此擋)

## 驗證紀律(每項共通)

- Docker build 通過 → game tester 跑正常玩家路徑 → 讀截圖目視 → commit + push。
- 測試用 env hook(`U1_TEST_DUNGEON` / `U1_TEST_ADJ_SPAWN`)正常遊玩零影響。
