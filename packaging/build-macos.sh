#!/usr/bin/env bash
# macOS 包(.app + zip):SDL2 系列「從源碼自編」→ 編遊戲 → dylibbundler 收 dylib → .app。
# 設計給 GitHub Actions macos runner(本機無 macOS 無法測;CI 驗證)。
# 預設 universal(arm64 + x86_64)。需:cmake、pkg-config、dylibbundler、git(brew 安裝)。
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
OUT="$ROOT/dist"; WORK="$ROOT/build-macos"; PREFIX="$WORK/prefix"
ARCHS="arm64;x86_64"
mkdir -p "$OUT" "$WORK" "$PREFIX"
export MACOSX_DEPLOYMENT_TARGET=11.0

SDL2_VER=release-2.30.9
IMG_VER=release-2.8.2
TTF_VER=release-2.22.0
MIX_VER=release-2.8.0

build_dep() {  # $1 repo  $2 tag  $3 extra-cmake-args
    local name="$1" tag="$2"; shift 2
    if [ ! -d "$WORK/src/$name" ]; then
        git clone --depth 1 --branch "$tag" "https://github.com/libsdl-org/$name" "$WORK/src/$name"
    fi
    cmake -S "$WORK/src/$name" -B "$WORK/b/$name" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DCMAKE_OSX_ARCHITECTURES="$ARCHS" \
        -DCMAKE_PREFIX_PATH="$PREFIX" \
        -DBUILD_SHARED_LIBS=ON "$@" >/dev/null
    cmake --build "$WORK/b/$name" -j"$(sysctl -n hw.ncpu)" >/dev/null
    cmake --install "$WORK/b/$name" >/dev/null
    echo "  built $name"
}

echo "== 1) 自編 SDL2 系列(vendored deps,universal)=="
build_dep SDL "$SDL2_VER"
build_dep SDL_image "$IMG_VER" -DSDL2IMAGE_VENDORED=ON -DSDL2IMAGE_SAMPLES=OFF
build_dep SDL_ttf   "$TTF_VER" -DSDL2TTF_VENDORED=ON  -DSDL2TTF_SAMPLES=OFF
build_dep SDL_mixer "$MIX_VER" -DSDL2MIXER_VENDORED=ON -DSDL2MIXER_SAMPLES=OFF

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
# 收 dylib 進 .app 並修 install_name(@executable_path 相對)
dylibbundler -od -b -x "$MACOS/u1_cht" -d "$MACOS/libs" -p "@executable_path/libs/" >/dev/null

echo "== 4) 打包 zip =="
( cd "$WORK" && zip -qry "$OUT/Ultima1-CHT-macos.zip" Ultima1-CHT.app )
cp packaging/README-dist.txt "$OUT/README-macos.txt"
echo "完成 → $OUT/Ultima1-CHT-macos.zip"
lipo -archs "$MACOS/u1_cht" 2>/dev/null && echo "(universal binary)"
