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
| E3 | **MSX** | .dsk(FAT12) | ✅ **完成**(Path B 反組譯 OUT.COM 破解格式):body `p*192+i`→VRAM `p*256+i`,SCREEN7 chunky 4bpp。MSXTILES=96 tiles。已建 `assets/tilesets/msx.png`(52槽,`build_msx_pack.py`)+ game tester 驗證 overworld 渲染(水/草/森林/玩家/中文狀態列正常)。工具:`decode_msxtiles.py`/`build_msx_pack.py` | ✅ 完成 |
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
- [ ] **MSX pack 弱對映槽**(`build_msx_pack.py` SLOT_MAP,低頻載具/怪物近似):slot 6 signpost(用要塞)、
      11 horse / 12 cart / 16 aircar(MSX 無明確馬/車/飛車 tile,用船/坐騎近似)、29 bear(用綠獸)。
      地形/結構/船/海怪/玩家/多數陸怪已精確。需精修時對照實機截圖再調 SLOT_MAP。

> 附帶可複用:FM Towns CD 音樂 `extract_fmtowns_cdda.py` → 對應「FM Towns 真音樂」backlog。
> 每平台流程:抽 → 轉 PNG(對齊 index)→ 載入驗證(截圖)→ commit。

### ★ E3 MSX openMSX — compact-safe 接續指南
> 文件:`docs/re/openmsx-{setup,build-experience}.md`、`docs/re/e3-msx.md`。工具:`tools/re/msx/`。
> 版權檔(BIOS/dsk/openMSX binary)在 `re_work/`(gitignore),repo clone 後需重建。

**已完成(可複用,別重做)**
- [x] openMSX 在 docker **編成**:`re_work/openMSX/derived/x86_64-linux-opt/bin/openmsx`(build-to-mount)。
- [x] 執行 image `docker/Dockerfile.msxrun`(含 runtime libs)。
- [x] BIOS:`archtaurus/RetroPieBIOS`/BIOS 抓 `MSX2/MSX2EXT/DISK/KANJI.ROM` → `re_work/msxbios/`
      + `re_work/omsx_share/systemroms/`(memory `retro-bios-source`)。sha1 對上 NMS8245。
- [x] 機器 config `tools/re/msx/U1MSX2.xml`(disk ROM window 修 0x0000)。
- [x] MSX .dsk = FAT12,抽出 `MSXTILES.BIN`(12292)等 → `re_work/msx/`。
- [x] **VDP 破解**:SCREEN 7 + palette **8 色 GRB**(`0黑1綠2紅3黃4藍5青6洋紅7白`,index=B<<2|R<<1|G)。
- [x] **遊戲流程摸清**:`OUT`→片頭→選單(a建角/b開始)→建角(左右增減/SPACE)→**雙磁碟存檔**(需空白角色碟,已建 `re_work/msx/CHARDISK.dsk`)。

**跑 openMSX(指令範本)**
```
docker run --rm -v "$PWD":/work -w /work u1-msxrun bash -c '
  export OPENMSX_SYSTEM_DATA=/work/re_work/openMSX/share OPENMSX_USER_DATA=/work/re_work/omsx_share
  xvfb-run -a /work/re_work/openMSX/derived/x86_64-linux-opt/bin/openmsx \
    -machine U1MSX2 -diska "/work/re_work/msx/<U1.dsk>" -script /work/re_work/<script>.tcl'
# Tcl: set throttle off; debug read_block "physical VRAM" 0 0x20000; screenshot -raw; type "..."; keymatrixdown/up
```

**待續(擇一,建議 B)**
- [ ] **A. 事件驅動 emulator 導航**:Tcl 偵測畫面(VRAM signature)→ 狀態機跑完建角+雙碟swap→overworld
      dump VRAM → 找 MSXTILES → 用 8色GRB palette + SCREEN7 佈局解碼 → 832×16 PNG。
      (卡點:絕對時間腳本不可靠,片頭長度飄移。)
- [ ] **B.(建議)反組譯 `OUT.COM`(Z80)**:它是 MSXTILES→VRAM 的 loader;借 `docs/re/6502-re-methodology.md`
      方法論(換 Z80,工具 `da65`→`z80dasm`/自寫遞迴下降)反追「檔→VRAM 搬運/轉換格式」。
      palette/mode 已知,只差佈局轉換 → 寫解碼器 → 套 E0 PNG AssetPack(`tools/build_fmtowns_pack.py` 為範本)。

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
