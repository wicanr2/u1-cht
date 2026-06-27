# Ultima I 繁中化 — 工作清單(WORKLIST)

> 進度追蹤。✅=完成並驗證 / 🔜=排程中 / ⬜=待辦 / 🧱=架構牆(大工程)。
> 每完成一項:tester 驗證 → commit → push → 勾選。更新日:2026-06-27(Phase A v1 完成)。

## ★ 本 fork 相對 open_ultima 上游的貢獻

> 以下都是上游 [`matiaslaino/open_ultima`](https://github.com/matiaslaino/open_ultima) **沒有做到**、由本專案新增的成果。
> 上游是極早期英文重製(世界地圖 + 基礎地牢 + 地面相鄰攻擊);本 fork 在其上補完並中文化。

**中文化 / 移植**
- 全畫面**繁體中文**(UI/訊息/狀態列/方向/怪名)+ **i18n 查表層**(`I18n::t/tf` + `assets/strings/<lang>.json`,可換語言)。
- **Linux 移植**(taoJSON→nlohmann、MSVC 例外可攜化)。
- **內部畫布 320→640**(中文 16px 銳利)+ Noto Serif CJK 抗鋸齒 + **全形寬度感知換行**。

**七平台美術(全自行逆向)**
- EGA / CGA / **FM Towns / MSX / PC-98 / Apple IIgs / Atari 8-bit** + VGA(EGA 擴色)tileset,`F1`/`PageDown` 熱鍵循環。
- 各自破解原版圖格式(IIgs LZSS 反組譯 65816、Atari $6400 1bpp charset、PC-98 4-plane planar、MSX SCREEN7、FM Towns chunky),
  完整 RE 紀錄於 `docs/re/`(含截圖 oracle 結構比對等方法)。

**各版音樂原生還原(零模擬器)**
- **MSX / PC-98 / FM Towns** 三版地圖 BGM:逆向各自序列格式(MCP / SCORE / EUPHONY,同作曲團隊)→ 自寫 2-op FM 合成器轉 ogg。
- 考證 Atari(原版無地圖 BGM)、IIgs(僅音效非 synthLAB 序列);見 `docs/music.md`。
- **音效系統**(Mix_Chunk)+ 程序生成音效;`M`/`F9` 音樂/音效獨立開關。

**遊戲系統 / 操作補完**
- **存檔系統**(`SaveGame`:玩家+設定序列化、F10 離開 autosave、F5 手動、啟動載入)。
- **離開鐵則**(F10/Ctrl+Q 確認框 + autosave、ESC 只取消)。
- **F1 指令說明畫面**、**F6 設定**(時間流速 / 怪物生成率 / **野外怪物追蹤開關**)。
- **食物 / 飢餓 tick**、回合制生怪 + `speed_pct`/`spawn_pct`。
- **怪物 AI**:地面忠於 1981(出現即攻擊不移動)+ 可選貪婪追蹤;地牢 beeline。
- **原版 U1 AI 考據**(逆向 1983 Atari)+ 跨平台 RE 方法論文件。

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
- [x] **F1 說明畫面 + F9 音效獨立開關**:F1 列出全部指令(modal);M=音樂、F9=音效兩者分開;
  tileset 循環移到 PageDown only。音效系統(Mix_Chunk)+ 程序生成 `assets/sfx/*.ogg`,接移動/攻擊。
- [x] **F6 設定擴充(參考 u2-cht `docs/GAME-MECHANICS.md` 的設定畫面)**:F6 三列、↑↓ 循環選列。
  - [x] **時間流速(time tick)**:`speed_pct` F6 標籤改「時間流速」、範圍 10–200%(clampPct)。
  - [x] **怪物生成率**:`spawn_pct`,F6 可調,10–200%。
  - [x] **★ 野外怪物自動追蹤玩家(開關)**:F6 第 3 列(`chase_monsters`,←→ 切換,預設關)。
        開 = `overworldMonsterTurn` 貪婪逐玩家走一步(避界外/山/水/玩家/他怪),關 = 原版不移動只相鄰反擊。
        config.json.example 已加;**執行期值,持久化待存檔系統**(同 speed/spawn)。
- [x] **食物 / 飢餓 tick**:每時間 tick 食物 −1;食物=0 時每回合扣 HP(參考 u2-cht 機制)
  - 掛在 `OverworldScreen::onStep` 與 `DungeonScreen` 的時間 tick;狀態列食物已顯示
- [~] **跨版本素材包(AssetPack,ADR 0001)**:可換不同平台外觀
  - [x] 第一步:tileset 變體 config 驅動(`tileset: ega/cga`)+ PageDown 循環(7 平台)
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
| E1 | **FM Towns** | Trilogy CD(GRAPH 已抽於 u7-cht) | ✅ **完成**:地形(UT1MAP)+ 載具(UT1TILE1)+ 怪物(UT1TILE0)全 52 槽。怪物槽原 cycling 跑進投射物→雜訊,已用明確 mapping 修正(`fix_fmtowns_monsters.py`)。8色palette+chunkyrev+32→16 | ✅ 完成 |
| E2 | **Apple IIgs** | woz(1994) | ✅ **全 52 槽 100% 真實 IIgs 像素**:反組譯 65816 破解 type 0x0001 = LZSS → `$4c00` 動態 dump 卡滑鼠牆 → **截圖 oracle 結構比對**反推出 overworld tileset = id08 tile 88-135。`build_iigs_pack.py`+`assets/tilesets/iigs.png`,EGA fallback 全除,game tester 驗證。完整見 `docs/re/apple-iigs-reverse-engineering.md` | ✅ 完成 |
| E3 | **MSX** | .dsk(FAT12) | ✅ **完成**(Path B 反組譯 OUT.COM 破解格式):body `p*192+i`→VRAM `p*256+i`,SCREEN7 chunky 4bpp。MSXTILES=96 tiles。已建 `assets/tilesets/msx.png`(52槽,`build_msx_pack.py`)+ game tester 驗證 overworld 渲染(水/草/森林/玩家/中文狀態列正常)。工具:`decode_msxtiles.py`/`build_msx_pack.py` | ✅ 完成 |
| E4 | **PC-98** | `org_game/msx/【PC98】….fdi` | ✅ **完成**:FDI(FAT12,NEC 2.00)→ mtools 抽 `EGCTILES.BIN`(30720B)。格式由自相關+結構比對破解:**60 個 32×32 tile,4-plane planar plane-major,512B/tile**(index=p0+p1·2+p2·4+p3·8)。palette 由 hg101 PC98 截圖反推=純 RGB 公式 **R=b2 G=b3 B=b1**(實質 8 色)。槽序對齊 `TileTypeLoader.cpp`(player=尾段 tile54,horse 後 pc98=engine−1),32→16 以 2×2 眾數降採樣。`build_pc98_pack.py`+`assets/tilesets/pc98.png`,game tester 驗證 overworld(鮮綠草/藍水/白袍玩家/中文狀態列)。 | ✅ 完成 |
| E5 | **Atari** | 1983 ATR(已抽檔)| ✅ **完成**(詳見 `docs/re/atari-tileset-re.md`):反組譯 `OUTMOVE.bin`→ tile 在 `$6400` charset 但為 **1bpp(每 byte 8 px)非 2bpp**(先前卡關主因);一個 tile = **char[i] 上半 + char[i+32] 下半**(8×16),共 **19 個 tile**。用實機圖反推驗證(grass=char1+33 等)+ palette(水藍/植被橄欖/結構生物灰)。`build_atari_pack.py`+`assets/tilesets/atari.png`,19→52 槽,game tester 進 overworld 渲染與 atari800-04 實機圖一致。 | ✅ 完成 |

> **DOS 原生 tileset(open_ultima 內建,gamedata BIN)**:
> - **EGA** ✅ `ega.png`(EGATILES.BIN,RowPlanar 4bpp,`tiles_to_png.py`)— 完整 52 槽,作各 pack fallback 基底。
> - **CGA** ✅ `cga.png`(CGATILES.BIN,Linear 2bpp 青/洋紅/白,`cga_tiles_to_png.py`)— 完整 52 槽真實 CGA。
> - **Tandy(T1K)** ✅ 已破解格式(`T1KTILES.BIN` = **chunky 4bpp,2px/byte 高 nibble 左**,Tandy/PCjr packed-pixel,`t1k_tiles_to_png.py`);但解出像素**與 EGA 100% 相同**(同套 16 色美術,只是 planar vs chunky 佈局差異)→ **不出貨成獨立 skin**(對玩家零視覺差異)。第一性判斷:可切換 skin 須有視覺區別。
> - **VGA** ✅ `vga.png`(EGA 重新染色:豐富基色 + 光影 + dither,15→1331 色;`build_vga_pack.py`)——
>   原版無 VGA,以 EGA 為底擴色域成 256 色感高彩版。
> **IIgs 載具/怪物**:✅ 已完成(LZSS 解 id08 88-135,見 E2)。EGA fallback 全除。
> **PC-98** ✅ `pc98.png`(EGCTILES 32×32 planar→16×16,純 RGB palette;`build_pc98_pack.py`)。

### E1 FM Towns 續作(已解碼,剩 palette+對應)
- [ ] U1 專屬 16 色 palette(RE U1 FM Towns 執行檔 / 對 hg101 FMTOWNS-06 實機採樣)— header 無內嵌 ColorMap
- [ ] 52 格對應(UT1MAP 地形 + UT1TILE1 物件 + UT1TILE0 怪物 → engine 槽)
- [ ] 32→16 downscale 拼 832×16 → 載入校驗(對實機)

### ★ 素材格式需修正 / 校準(實作中發現,逐項修)
- [x] **Tandy(T1K)**:`T1KTILES.BIN` 與 EGA 同大小(6656B)不同格式 → 破解 = **chunky 4bpp(2px/byte,高 nibble 左)**,
      Tandy/PCjr packed-pixel;`t1k_tiles_to_png.py`。**解出像素與 EGA 100% 相同**(同套美術)→ 不出貨成獨立 skin。記:同大小≠同格式;不同格式也可能=同像素。
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

- [x] **存檔系統**(`SaveGame` → `save.json`):序列化玩家位置/數值 + F6 設定;F10「Y 離開」autosave、F5 手動存檔、啟動自動載入。截圖驗證載入還原生命/食物/經驗/金幣/位置/設定。**F6 設定持久化隨此完成**。
- [x] **i18n 查表層**(`I18n::t/tf` + `assets/strings/zh-Hant.json`):~97 個用戶可見字串全外移成 key→譯文;插值用 `{0}` 佔位符(`tf`)。全檔遷移、缺 key 回退顯示 key。截圖驗證 F1/狀態列/訊息正確。
- [x] **`CommandDisplay` 全形寬度感知換行**:依顯示寬度斷行(全形 2 / 半形 1 單位,上限 52),只在 UTF-8 字元邊界切。取代原 byte 數換行(中文行被切太短)。
- [x] **各版本音樂原生還原**(MSX/PC-98/FM Towns 三版,自寫合成器、零模擬器;見 `docs/music.md`)。
      Atari/DOS 原版無 BGM、IIgs 僅音效(已考證)。overworld 曲目已用音級比對確認(PC-98=song1)。
- [ ] 打包(AppImage / Windows)— **使用者指示不急著打包**

## C2. 城鎮內容 + 戰鬥深度(規劃見 [`docs/plan-town-combat.md`](docs/plan-town-combat.md))🔜

> 資料三要件已確認(手冊/素材/RE)。從上游「城鎮空殼+固定傷害」補完。

- [x] **Phase A v1 — 商店系統(核心可玩)**:`Player` 加 6 屬性 + 武器[16]/防具[8]/法術[8] 物品欄 + 裝備;
      `ItemCatalog`(品項+價格,i18n);商店 modal(`B` 城鎮開→類別→品項→購買/裝備/食物+50/金幣不足);
      `SaveGame` 序列化。截圖驗證、regression OK。
- [ ] **Phase A.2 — 商店精修(authentic 店員定位)**:RE 城鎮格式(index 51-63 解碼 + 店員放置)→
      站到招牌旁(WEAPONS/ARMOUR/MAGIC/FOOD/PUB)自動判定店家類型,取代 `B` 類別選單。
- [ ] **Phase B — 戰鬥深度**:傷害接裝備(攻 = f(武器,力量)、受 = f(怪,防具));XP→升級(HP/屬性);
      死亡→回 Lord British 城堡復活(扣資源)。`ItemCatalog::weaponPower/armorDefense` 已備。
- [ ] **Phase C — 城堡/國王/法術/酒館**:國王給任務+領獎;地牢施法(買來的法術:飛彈/致死/升降梯/開鎖);酒館線索。

## D. 架構牆 / 長期 🧱

- [ ] 逆向常駐引擎逐行確認怪物移動 6502 routine(模擬鏈載入→重定位→$3000 引擎)
  - 續追路徑見 `docs/re/README.md`;目前 AI 行為已足以忠實實作(B/C 不被此擋)

## 驗證紀律(每項共通)

- Docker build 通過 → game tester 跑正常玩家路徑 → 讀截圖目視 → commit + push。
- 測試用 env hook(`U1_TEST_DUNGEON` / `U1_TEST_ADJ_SPAWN`)正常遊玩零影響。
