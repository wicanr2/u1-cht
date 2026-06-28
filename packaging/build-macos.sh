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

build_dep() {  # $1 repo  $2 tag  $3 extra-cmake-args
    local name="$1" tag="$2"; shift 2
    if [ ! -d "$WORK/src/$name" ]; then
        # --recurse-submodules:SDL_image/ttf/mixer 的 VENDORED 相依(libpng/freetype/
        # harfbuzz/libogg/vorbis…)以 git submodule 放在 external/,不抓會編不出來。
        git clone --depth 1 --recurse-submodules --shallow-submodules \
            --branch "$tag" "https://github.com/libsdl-org/$name" "$WORK/src/$name"
    fi
    cmake -S "$WORK/src/$name" -B "$WORK/b/$name" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DCMAKE_OSX_ARCHITECTURES="$ARCHS" \
        -DCMAKE_PREFIX_PATH="$PREFIX" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DBUILD_SHARED_LIBS=ON "$@" >/dev/null
    cmake --build "$WORK/b/$name" -j"$(sysctl -n hw.ncpu)" >/dev/null
    cmake --install "$WORK/b/$name" >/dev/null
    echo "  built $name"
}

echo "== 1) 自編 SDL2 系列(vendored deps;只開遊戲用得到的格式以免巨庫拖垮編譯)=="
build_dep SDL "$SDL2_VER"
# SDL_image:遊戲只載 PNG → 關掉 AVIF(dav1d)/JXL(libjxl)/WEBP/TIF/JPG 這些
# 從源碼編很慢的大庫(正是 universal/單架構都 >30min 的真兇),只留 PNG。
build_dep SDL_image "$IMG_VER" -DSDL2IMAGE_VENDORED=ON -DSDL2IMAGE_SAMPLES=OFF \
    -DSDL2IMAGE_PNG=ON \
    -DSDL2IMAGE_AVIF=OFF -DSDL2IMAGE_JXL=OFF -DSDL2IMAGE_WEBP=OFF \
    -DSDL2IMAGE_TIF=OFF -DSDL2IMAGE_JPG=OFF
build_dep SDL_ttf   "$TTF_VER" -DSDL2TTF_VENDORED=ON  -DSDL2TTF_SAMPLES=OFF
# SDL_mixer:遊戲只用 OGG(Vorbis)+ WAV → VORBIS 後端用內建 STB(免外部 vorbis/ogg/tremor,
# 編更快;注意 SDL2MIXER_VORBIS 是 enum=STB|TREMOR|VORBISFILE,不可填 ON)。關 FLAC/MOD/MIDI/OPUS/MP3。
build_dep SDL_mixer "$MIX_VER" -DSDL2MIXER_VENDORED=ON -DSDL2MIXER_SAMPLES=OFF \
    -DSDL2MIXER_VORBIS=STB -DSDL2MIXER_WAVE=ON \
    -DSDL2MIXER_FLAC=OFF -DSDL2MIXER_MOD=OFF -DSDL2MIXER_MIDI=OFF \
    -DSDL2MIXER_OPUS=OFF -DSDL2MIXER_MP3=OFF

echo "== 2) 編遊戲(指向自編 SDL2)=="
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
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
dylibbundler -od -b -x "$MACOS/u1_cht" -d "$APP/Contents/Frameworks/" \
    -p "@executable_path/../Frameworks/" >/dev/null
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
# .dmg 雙保險(失敗不致命)
hdiutil create -volname "Ultima1-CHT" -srcfolder "$APP" -ov -format UDZO \
    "$OUT/Ultima1-CHT-macos-${ARCHTAG}.dmg" >/dev/null 2>&1 || true
echo "完成 → $OUT/Ultima1-CHT-macos-${ARCHTAG}.zip"
lipo -archs "$MACOS/u1_cht" 2>/dev/null && echo "(以上為 binary 架構)"
