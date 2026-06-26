# 中文化技術筆記

## 文字管線(已接通)
- **單一渲染出口**:`LTexture::loadFromRenderedText`。已從 `TTF_RenderText_Solid`
  改為 **`TTF_RenderUTF8_Solid`**(ASCII 不受影響,額外支援中文碼點)。
- **字型分流**:`Fonts::standard()`(8px PC_Senior,ASCII/數字)與 `Fonts::cjk()`
  (Noto CJK,中文)。`cjk()` 載入失敗時回退 standard。候選路徑見 `Fonts.cpp`
  (repo `assets/fonts/` 優先 → 系統 `/usr/share/fonts/.../NotoSansCJK-Regular.ttc`)。
- **翻譯方式(現階段)**:**原始碼內聯**(直接把英文字面換中文),非 JSON 查表。
  完整 i18n 查表層(`I18n::t(key)`)留待後續重構;`assets/strings/zh-Hant/ui.json`
  為對照表 / 未來遷移來源。
- **已翻譯字串**(現有畫面全覆蓋):
  - 狀態列:生命/食物/經驗/金幣(`PlayerStatusDisplay`)
  - 世界地圖:山脈/水域/受阻/擊殺/匕首攻擊/方向(`OverworldScreen`、`CardinalPoint`)
  - 地牢:前進/轉向/攀爬(K-Limb)/層數/戰鬥命中傷害/死亡(`DungeonScreen`)
  - 敵人名:遊蕩術士/盜賊/巨鼠
- **UTF-8 換行暫行修正**:`CommandDisplay` 切行時對齊 UTF-8 字元邊界(避免切壞中文);
  全形寬度感知換行於拉畫布階段完整處理。

## 關鍵決策:拉高內部畫布,不縮字(✅ 已實作)
**已完成**(`tests/snapshots/out/phaseA-canvas2.png`):中文狀態列清晰可讀,世界 crisp。
- `main.cpp`:世界畫進 320×200 離屏 target(`gWorldTarget`,世界座標完全不動)→
  nearest 放大到 640×400 邏輯畫布。UI(狀態列/指令列)在 640 空間以 **2× 座標 + 16px CJK** 繪製。
- `Fonts`:`cjk()`=12px(in-target 320,地牢標籤用)、`cjkUi()`=16px(640 覆蓋層)。
- 面板改 `SDL_RenderFillRect` 黑底(取代未縮放的 _background 貼圖)。
- 待續:**地牢標籤**(方位/層數)仍在 target 內 12px,需比照搬到 640 覆蓋層;
  `CommandDisplay` 全形寬度換行(`MAX_LINE_CHARS`)精修。

### 原始決策脈絡(實證)
- 現況:`main.cpp` `SDL_RenderSetLogicalSize(gRenderer, 320, 200)` + nearest 放大到視窗
  (config `960×600` = 3×)。所有繪製(含文字)都在 320×200 邏輯空間,整體放大。
- **實測**(`tests/snapshots/out/phase2-cjk.png`):12px 中文畫在 320×200 內、再放大 3×
  → 筆畫糊、行距(8px)重疊、不可讀。數字尚可。
- **結論(對齊 retro-cjk-hires-canvas 鐵則)**:中文筆畫多,縮進 ~8px 邏輯字必糊。
  正解是**拉高引擎內部畫布**(320×200 → 640×400 乾淨 2×,或 640×480 方正),
  底圖 nearest 放大保銳利,中文用 16/24px 畫在放大後畫布。

### 待實作(Phase 2 續)
1. 畫布常數 320×200 → 640×400(`Constants.h` `GAME_VIEW_*`、`main.cpp` LogicalSize)。
2. tile/sprite 繪製與 UI 面板座標 2× 重映射(`CommandDisplay POSITION_Y=160`、
   `PlayerStatusDisplay` padding、地牢線框等硬編碼座標)。
3. CJK 字級提到 16(或 24),`CommandDisplay MAX_LINE_CHARS` 改全形寬度感知換行。
4. 重新截圖驗證中文清晰、不溢框、不重疊。
> 取捨:640×400 是乾淨 2×(底圖最銳利);640×480 方正但底圖需 400→480 比例映射。
> 預設先 640×400 整數倍,排版衝擊最小。

## 字庫重建(後續)
- 專案指示「用優質系統中文 TTF 重建字庫」:從 Noto CJK 烘出**只含所用字**的子集 TTF
  放 `assets/fonts/u1-cjk.ttf`,縮小體積、利於散布(參考 retro-game-remake 的
  `build_cjk_font.py`)。目前先用系統完整 Noto CJK 驗證。
