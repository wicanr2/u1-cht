#!/usr/bin/env bash
# macOS 包(.app + zip):SDL2 系列「從源碼自編」→ 編遊戲 → dylibbundler 收 dylib → .app。
# 設計給 GitHub Actions macos runner(本機無 macOS 無法測;CI 驗證)。
# 預設 universal(arm64 + x86_64)。需:cmake、pkg-config、dylibbundler、git(brew 安裝)。
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
OUT="$ROOT/dist"; WORK="$ROOT/build-macos"; PREFIX="$WORK/prefix"
# 預設 universal(單一 runner 通吃兩架構);MAC_ARCH=arm64|x86_64 則只建該架構
# (u4-cht 矩陣策略:雙 runner 各建單架構,避免 universal 編 vendored 相依太慢/卡)。
ARCHS="${MAC_ARCH:-arm64;x86_64}"
ARCHTAG="${MAC_ARCH:-universal}"; ARCHTAG="${ARCHTAG//;/-}"
mkdir -p "$OUT" "$WORK" "$PREFIX"
export MACOSX_DEPLOYMENT_TARGET=11.0

SDL2_VER=release-2.30.9
IMG_VER=release-2.8.2
TTF_VER=release-2.22.0
MIX_VER=release-2.8.0

build_dep() {  # $1 repo  $2 tag  $3 "需要的 submodule(空格分隔,空=無)"  $4.. cmake args
    local name="$1" tag="$2" subs="$3"; shift 3
    if [ ! -d "$WORK/src/$name" ]; then
        # ⚠ 不用 --recurse-submodules:它會把 dav1d/libjxl/libavif/libwebp/harfbuzz/flac/opus
        # 這些「我們 build 時關掉、但 clone 仍會抓」的巨大 repo 全下載(macOS CI 上正是卡死主因)。
        # 改成只 init 真正需要的 submodule(PNG→zlib+libpng、ttf→freetype、mixer STB→無)。
        git clone --depth 1 --branch "$tag" "https://github.com/libsdl-org/$name" "$WORK/src/$name"
        if [ -n "$subs" ]; then
            ( cd "$WORK/src/$name" && git submodule update --init --depth 1 $subs )
        fi
    fi
    echo "  [$(date +%H:%M:%S)] config $name"
    cmake -S "$WORK/src/$name" -B "$WORK/b/$name" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DCMAKE_OSX_ARCHITECTURES="$ARCHS" \
        -DCMAKE_PREFIX_PATH="$PREFIX" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DBUILD_SHARED_LIBS=ON "$@" >/dev/null
    echo "  [$(date +%H:%M:%S)] build $name"
    cmake --build "$WORK/b/$name" -j"$(sysctl -n hw.ncpu)" >/dev/null
    cmake --install "$WORK/b/$name" >/dev/null
    echo "  [$(date +%H:%M:%S)] OK $name"
}

if [ "${MAC_USE_BREW:-0}" = "1" ]; then
  # ── 應急 opt-in:brew 預編(預設不走;此專案要求 SDL2 自編)──────────────
  echo "== 1) 用 brew 預編 SDL2(MAC_USE_BREW=1)=="
  BREW="$(brew --prefix)"
  brew list sdl2 >/dev/null 2>&1 || brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer
  export PKG_CONFIG_PATH="$BREW/lib/pkgconfig"
else
  # ── 預設:從源碼自編 SDL2 系列(只 init 需要的 submodule,跳過巨庫 clone)──
  echo "== 1) 自編 SDL2 系列(只開遊戲用得到的格式 + 選擇性 submodule)=="
  build_dep SDL       "$SDL2_VER" ""
  # SDL_image 只要 PNG → 只 init zlib + libpng(跳過 dav1d/libavif/libjxl/libwebp/libtiff/libjpeg)
  build_dep SDL_image "$IMG_VER" "external/zlib external/libpng" \
      -DSDL2IMAGE_VENDORED=ON -DSDL2IMAGE_SAMPLES=OFF \
      -DSDL2IMAGE_PNG=ON -DSDL2IMAGE_AVIF=OFF -DSDL2IMAGE_JXL=OFF -DSDL2IMAGE_WEBP=OFF \
      -DSDL2IMAGE_TIF=OFF -DSDL2IMAGE_JPG=OFF
  # SDL_ttf 只要 freetype(harfbuzz off → 跳過 harfbuzz submodule)
  build_dep SDL_ttf   "$TTF_VER" "external/freetype" \
      -DSDL2TTF_VENDORED=ON -DSDL2TTF_SAMPLES=OFF -DSDL2TTF_HARFBUZZ=OFF
  # SDL_mixer:VORBIS=STB 內建、WAVE 內建 → 關掉所有需 submodule 的格式(含 WavPack/GME,
  # 它們預設 ON 且 VENDORED 會要 external/wavpack、external/libgme)→ 不需任何 submodule
  build_dep SDL_mixer "$MIX_VER" "" \
      -DSDL2MIXER_VENDORED=ON -DSDL2MIXER_SAMPLES=OFF \
      -DSDL2MIXER_VORBIS=STB -DSDL2MIXER_WAVE=ON \
      -DSDL2MIXER_FLAC=OFF -DSDL2MIXER_MOD=OFF -DSDL2MIXER_MIDI=OFF \
      -DSDL2MIXER_OPUS=OFF -DSDL2MIXER_MP3=OFF -DSDL2MIXER_WAVPACK=OFF -DSDL2MIXER_GME=OFF
  export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
fi

echo "== 2) 編遊戲(指向 SDL2)=="
rm -rf "$WORK/cmake"
cmake -S . -B "$WORK/cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="$ARCHS" \
    -DCMAKE_PREFIX_PATH="$PREFIX" >/dev/null
cmake --build "$WORK/cmake" -j"$(sysctl -n hw.ncpu)" >/dev/null

echo "== 3) 組 .app(binary 與資產同層;dylibbundler 收 dylib)=="
APP="$WORK/Ultima1-CHT.app"; rm -rf "$APP"
MACOS="$APP/Contents/MacOS"
mkdir -p "$MACOS" "$APP/Contents/Resources"
cp "$WORK/cmake/u1_cht" "$MACOS/"
cp -r assets resources "$MACOS/"
cp packaging/dist-config.json "$MACOS/config.json"
cat > "$APP/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleName</key><string>Ultima1-CHT</string>
  <key>CFBundleExecutable</key><string>u1_cht</string>
  <key>CFBundleIdentifier</key><string>tw.lcy.ultima1cht</string>
  <key>CFBundleVersion</key><string>1.0</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST
# 收 dylib 進 .app/Contents/Frameworks 並修 install_name(u4-cht 慣例)
# -s "$PREFIX/lib":自編 SDL 的 install name 是 @rpath/...,必須給搜尋路徑,
#   否則 dylibbundler 找不到實體會進互動問答(CI 無 stdin → 無限「Try again」hang 40min)。
# </dev/null:保險,真找不到時 fail-fast 而非卡死等輸入。
dylibbundler -od -b -x "$MACOS/u1_cht" -d "$APP/Contents/Frameworks/" \
    -p "@executable_path/../Frameworks/" -s "$PREFIX/lib" >/dev/null </dev/null
# ad-hoc 簽章(降低 Gatekeeper 直接拒;u4-cht 經驗)
codesign --force --deep --sign - "$APP" || true

echo "== 4) 打包 zip(ditto 保留 .app 結構/符號連結/簽章;非 zip)=="
cat > "$WORK/安裝說明.txt" <<'NOTE'
創世紀一代繁體中文 remake(macOS）
1. 解壓得到 Ultima1-CHT.app。把原版 *.BIN 放進 app 內 Contents/MacOS/gamedata/
   (或與 app 同層的 gamedata/)。
2. 首次開啟若被 Gatekeeper 擋:右鍵「打開」→「打開」,或終端機:
   xattr -dr com.apple.quarantine Ultima1-CHT.app
3. 遊戲內按 F1 看完整指令。存檔在 ~/Library/Application Support/LCY/Ultima1-CHT/。
NOTE
rm -f "$OUT/Ultima1-CHT-macos-${ARCHTAG}.zip"
( cd "$WORK" && ditto -c -k --sequesterRsrc --keepParent Ultima1-CHT.app "$OUT/Ultima1-CHT-macos-${ARCHTAG}.zip" \
  && zip -qj "$OUT/Ultima1-CHT-macos-${ARCHTAG}.zip" 安裝說明.txt )
# 註:不做 .dmg —— hdiutil create 在 CI/headless 會 hang(本機 SDL build 僅數十秒,
# 50min 卡死定位於此)。zip(ditto)即 macOS 主交付;需要 .dmg 可本機另跑。
echo "完成 → $OUT/Ultima1-CHT-macos-${ARCHTAG}.zip"
lipo -archs "$MACOS/u1_cht" 2>/dev/null && echo "(以上為 binary 架構)"
