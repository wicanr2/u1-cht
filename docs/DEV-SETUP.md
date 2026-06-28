# 開發環境與打包(DEV-SETUP)

創世紀一代繁體中文 remake 的建置、執行、測試、打包說明。

## 0. 取得原版資料(必要)

本專案**不含版權遊戲資料**。請自備原版 Ultima I 的 `*.BIN`,放進 repo 根的 `gamedata/`:

```
gamedata/MAP.BIN  EGATILES.BIN  CGATILES.BIN  EGATOWN.BIN  CGATOWN.BIN  TCD.BIN
```

並從範本建立設定檔:`cp config.json.example config.json`(預設已指向 `./gamedata/`)。

## 1. 建置(Docker first,建議)

一律在容器內 build/test,不污染系統(專案硬規則)。

```bash
# 建一次映像
docker build -t u1-cht -f docker/Dockerfile .

# 建置
docker run --rm -v "$PWD":/work -w /work u1-cht \
  bash -c 'cmake -S . -B build && cmake --build build -j4'

# 執行(需 X 顯示;或見 §3 測試)
./build/u1_cht
```

依賴(`docker/Dockerfile` 已含):`build-essential cmake pkg-config`、
`libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev`、`nlohmann-json3-dev`。
**SDL2_gfx 已 vendored**(`third_party/SDL2_gfx/`,直接編入),不需系統套件。

## 2. 原生 Linux 建置(不走 Docker)

```bash
sudo apt-get install -y build-essential cmake pkg-config \
  libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev nlohmann-json3-dev
cmake -S . -B build && cmake --build build -j$(nproc)
./build/u1_cht
```

## 3. 測試(game_tester)

`tools/game_tester.sh` 在 Xvfb 下驅動正常玩家路徑 + RPG 系統煙霧,每步截圖供目視。

```bash
docker run --rm -v "$PWD":/work -w /work u1-cht bash tools/game_tester.sh
# 輸出截圖:tests/snapshots/out/*.png;機械 PASS/FAIL 看 rpg_smoke 行
```

測試環境變數:`U1_SAVE_DIR=.`(存檔寫 CWD)、`U1_SKIP_CC=1`(跳建角)、
`U1_TEST_DUNGEON/TOWN/CASTLE/SPACE/ENDGAME=1`(直接進該場景)。

## 4. 路徑與存檔(打包穩健性)

程式啟動會 `chdir(SDL_GetBasePath())` → `assets/ resources/ config.json` 一律在
執行檔旁找。版權 `gamedata/` 先找執行檔旁、再找啟動目錄(AppImage 旁放 BIN 可行)。
存檔寫使用者可寫目錄(`SDL_GetPrefPath`:Windows `%APPDATA%\LCY\Ultima1-CHT`、
macOS `~/Library/Application Support/`、Linux `~/.local/share/LCY/`);
`U1_SAVE_DIR` 可覆寫。

## 5. 打包

三平台腳本在 `packaging/`,產出到 `dist/`。

| 平台 | 指令 | 產出 |
|---|---|---|
| Linux AppImage | `docker run --rm -v "$PWD":/work -w /work u1-cht bash packaging/build-appimage.sh` | `Ultima1-CHT-x86_64.AppImage` |
| Windows x64 | `bash packaging/build-windows.sh`(需 `g++-mingw-w64-x86-64`) | `Ultima1-CHT-windows-x64.zip` |
| macOS | `bash packaging/build-macos.sh`(僅 macOS runner;SDL2 自源碼編) | `Ultima1-CHT-macos.zip` |

- **Windows**:MinGW 交叉編譯,自動下載 SDL2 系列 MinGW devel、收 DLL、靜態連 libgcc/libstdc++。
- **macOS**:SDL2/image/ttf/mixer 從源碼自編,`dylibbundler` 收 dylib + ad-hoc 簽章 + `ditto` 打包成 `.app`。
  - **策略**:**universal 優先**(`macos-latest` arm64 runner,`CMAKE_OSX_ARCHITECTURES=arm64;x86_64`,一個 `.app` 通吃 Apple Silicon + Intel)。
  - **universal >30 分鐘未完成 / 失敗 → 退雙 runner 矩陣**(u4-cht 證實):`macos-14`(arm64)+ `macos-15-intel`(x86_64),`build-macos.sh` 認 `MAC_ARCH=arm64|x86_64` 各建單架構。
  - ⚠ `macos-13` 已 **2025-12-04 退役**,勿用。
- **CJK 字型**:`assets/fonts/u1-cjk.ttf`(子集 Noto Serif CJK)已隨庫,打包自帶,無系統字型依賴。

### CI(GitHub Actions)

`.github/workflows/release.yml`:推 `v*` tag 或手動觸發 → 三平台並行打包、上傳 artifact,
tag 時建 GitHub Release。Linux/Windows 在 `ubuntu-22.04`(較舊 glibc = AppImage 相容性佳),
macOS 在 `macos-latest`。

```bash
git tag v1.0 && git push origin v1.0     # 觸發 release
```
