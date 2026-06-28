# Android 移植規劃(操作界面 + 編譯)

> 狀態:**規劃**(尚未實作)。本遊戲是 C++17/SDL2,Android 走 **SDL2 官方 android-project**
> 路線(非 u4-cht 的 xu4/GLV 路線)。以下為實作前的工程規格。

## 0. 結論先行

- **能做**:SDL2 對 Android 一等支援,遊戲邏輯/繪圖/音訊不動;主要工作是 ①觸控操作層 ②資產 IO 調整 ③Gradle/NDK 打包。
- **三塊工作量**:觸控 UI(中)、資產讀取改 SDL_RWops(小,2–3 檔)、build 系統(中,一次性)。
- **版權**:`gamedata/*.BIN` 不入 APK;使用者側載到 App 外部目錄。字型/tileset/音效已自帶可入 APK。

---

## 1. 操作界面(觸控)

遊戲原本純鍵盤(方向鍵 + `A/E/K/B/T/C/Z/L` + F 鍵)。Android 疊一層**螢幕觸控控制**,
命中測試後用 `SDL_PushEvent` 合成對應的 `SDL_KEYDOWN/UP` → **既有 `handle()` 完全不改**。

### 版面(橫向 landscape,雙拇指)
```
┌──────────────────────────────────────────────┐
│ [F1?] [F6⚙] [F5💾]              [M♪][F9🔊]    │  ← 頂列小鈕(說明/設定/存檔/音訊)
│                                               │
│        ╭───╮                      (A)攻擊      │
│      ◄─┤ ↑ ├─►   遊戲畫面          (E)進入/離開 │
│        │↓  │                      (K)攀爬      │
│        ╰───╯                  (B)(T)(C)(Z)(L) │  ← 情境鈕
│   方向 D-pad(移動/轉向/選單)      動作鈕        │
└──────────────────────────────────────────────┘
```

### 鍵位對映(觸控 → 合成鍵)
| 觸控元件 | 合成鍵 | 用途(世界/選單共用)|
|---|---|---|
| D-pad ↑↓←→ | 方向鍵 | 移動・轉向 / 選單上下左右調整 |
| **A**(主鈕)| `A` 或 `Enter` | 攻擊 / 選單確定(選單時送 `Enter`)|
| **B**(返回)| `ESC` | 取消・返回上一層(離開鐵則)|
| E / K | `E` / `K` | 進入離開・攀爬 |
| 情境列 B T C Z L | `B/T/C/Z/L` | 商店/國王/施法/屬性/升空(依場景顯示)|
| 頂列 | `F1/F5/F6/F9/M` | 說明/存檔/設定/音效/音樂 |

> 選單模式(shop/king/cast/ztats/settings/建角/離開框)只需 **D-pad + A(=Enter) + B(=ESC)**,
> 與既有鍵盤流程一致 → 一套觸控層通吃遊戲與選單。

### 實作
- 新 `src/android/TouchControls.{h,cpp}`(僅 Android build 編入):用 SDL 基本圖元繪半透明按鈕;
  `handleTouch(SDL_FINGERDOWN/UP/MOTION)` 命中 → `pushKey(SDLK_xxx)`(SDL_PushEvent 合成 keydown+keyup)。
- 主迴圈在 640×400 覆蓋層(同現有 modal 繪法)畫控制層;`#ifdef __ANDROID__` 才啟用。
- 情境列依 `gameContext->getCurrentScreen()` + 是否 inCastle 動態顯示相關鈕(減少誤觸)。
- 可選:左下角虛擬搖桿替代 D-pad(類比 → 四向量化)。

---

## 2. 資產 IO 調整(Android 必要)

Android 的 APK `assets/` **不是真實檔案系統**:`SDL_RWFromFile` 會自動走 `AAssetManager` 讀到,
但 `std::ifstream` **讀不到**。盤點:

| 來源 | 載入方式 | Android |
|---|---|---|
| tileset PNG / 字型 TTF / 音效音樂 | `IMG_Load`/`TTF_OpenFont`/`Mix_Load*`(內部 SDL_RWops)| ✅ 直接可讀 APK assets |
| 地圖・城鎮 BIN(gamedata)| `SDL_RWFromFile` | ✅ 機制可,但**檔案側載**(見下)|
| **config.json** | `std::ifstream`(`Configuration.cpp`)| ❌ 需改 |
| **語言 json** | `std::ifstream`(`I18n.cpp`)| ❌ 需改 |
| save.json | `std::ifstream` + `SDL_GetPrefPath` | ✅(寫內部儲存,真實 FS)|

### 要改的(小,bounded)
1. **共用讀檔 helper**:`readFileToString(path)` 用 `SDL_RWFromFile`+`SDL_RWread`(桌面/Android 一致)。
   `Configuration::init`、`I18n::init` 改用它讀 config.json / 語言 json。
2. **`chdir(SDL_GetBasePath())`**:Android 上 `SDL_GetBasePath()` 回 NULL → `#ifdef __ANDROID__` 跳過;
   資產用相對路徑經 SDL_RWops 走 AAssetManager。需確認 APK 內資產佈局與程式用的路徑前綴一致
   (Gradle 把 `assets/`、`resources/`、`config.json` 放進 `app/src/main/assets/`,路徑去掉 `./`)。
3. **gamedata(版權 BIN)側載**:Android 用 `SDL_AndroidGetExternalStoragePath()`
   (= `/sdcard/Android/data/<pkg>/files/`)作 gamedata 根。`Configuration` 的 gamedata 解析
   在 Android 多加這個候選目錄;該目錄是真實 FS,`ifstream`/`SDL_RWFromFile` 皆可。首次啟動若缺 BIN,
   顯示中文提示「請把原版 *.BIN 放到 …/files/gamedata/」。

---

## 3. 編譯方式(SDL2 android-project + Gradle/NDK)

### 結構
```
android/
  app/
    src/main/
      AndroidManifest.xml           # 橫向、SDLActivity 入口
      assets/  → assets/ resources/ config.json(打包進 APK)
      java/org/libsdl/app/…         # SDL2 內附 Java(SDLActivity)
    jni/
      SDL2/ SDL2_image/ SDL2_ttf/ SDL2_mixer/   # 各 SDL 源碼(submodule 或解壓)
      src/  → 本遊戲 CMakeLists(複用,加 third_party/SDL2_gfx)
      CMakeLists.txt                # 聚合:add_subdirectory(SDL2…) + 本遊戲
  build.gradle / settings.gradle    # externalNativeBuild cmake + NDK
```

- **SDL2 系列**:用官方 release 源碼樹放 `jni/`(SDL android-project 慣例),Gradle 的
  `externalNativeBuild { cmake { } }` 一起編 → 產 `libSDL2.so` + `libmain.so`(本遊戲)。
- **本遊戲 CMake 複用**:現有 `CMakeLists.txt` 已 vendored SDL2_gfx + nlohmann,Android 端
  改用 SDL 提供的 target(`SDL2::SDL2`…)而非 pkg-config;以 `if(ANDROID)` 分支處理。
- **ABI**:`arm64-v8a`(主)+ 可選 `armeabi-v7a`、`x86_64`(模擬器)。
- **min SDK**:21+(SDL2 要求);target 32+。

### CI(GitHub Actions,參考 u4-cht build-android.yml 骨架)
```yaml
- uses: actions/setup-java@v4         # temurin 17
- uses: android-actions/setup-android@v3
- run: sdkmanager "ndk;26.x" "platforms;android-34" "build-tools;34.0.0"
- run: cd android && ./gradlew assembleDebug --no-daemon
- uses: actions/upload-artifact@v4    # *-debug.apk
```
（簽章:debug APK 自簽即可安裝;正式版另設 keystore signingConfig。）

---

## 4. 分階段

| 階段 | 內容 | 產出 |
|---|---|---|
| **P0 骨架** | SDL2 android-project + Gradle/NDK 編出能跑的空殼 APK(SDL 開窗) | 可安裝 APK |
| **P1 資產 IO** | readFileToString(RWops)、跳過 chdir、gamedata 外部目錄、assets 入 APK | 載入到世界地圖畫面 |
| **P2 觸控 UI** | TouchControls 疊層 + 合成鍵;D-pad + 動作/情境/頂列鈕 | 可單手把玩完整流程 |
| **P3 收尾** | 橫向鎖定、安全區(瀏海)、震動回饋、APK 簽章、CI workflow | 可發佈 debug/release APK |

## 5. 風險

- **資產路徑前綴**:`./assets/...` 在 Android RWops 的解析需實測(可能要去 `./`);P1 第一個要驗的點。
- **觸控誤觸**:情境鈕依場景動態顯示 + 適當間距/大小;需實機調。
- **gamedata 側載 UX**:Android 11+ scoped storage,用 App 專屬外部目錄(免權限)最穩。
- **效能**:320×200→放大 + SDL_ttf,負載低,中階機無虞。
- **首次 CI 驗證**:NDK 版本、SDL 源碼樹路徑、Gradle 版本對齊需一次 CI 跑通(同 u4-cht「scaffold 需首跑驗證」)。

---

## 附:與桌面共用程度
遊戲邏輯/繪圖/音訊/i18n/存檔(PrefPath)**全共用**;Android 專屬僅 `TouchControls`、
資產 IO 的 `#ifdef __ANDROID__` 分支、build 腳本。桌面四平台(Win/macOS/Linux）不受影響。
