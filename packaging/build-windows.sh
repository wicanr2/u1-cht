#!/usr/bin/env bash
# Windows x86_64 包(MinGW-w64 交叉編譯):下載 SDL2 系列 MinGW devel → 交叉編譯 →
# 收 DLL → 打包 zip(exe + DLL + assets + resources + config)。
# 於 Ubuntu 容器內、repo 根執行(需 mingw-w64 / cmake / wget):
#   docker run --rm -v "$PWD":/work -w /work u1-cht bash packaging/build-windows.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
OUT="$ROOT/dist"; WORK="$ROOT/build-windows"; SYS="$WORK/sysroot/x86_64-w64-mingw32"
mkdir -p "$OUT" "$WORK/dl" "$SYS"

SDL2_VER=2.30.9; IMG_VER=2.8.2; TTF_VER=2.22.0; MIX_VER=2.8.0
base_sdl=https://github.com/libsdl-org

fetch() {  # $1 url  $2 outfile
    [ -f "$WORK/dl/$2" ] || wget -qO "$WORK/dl/$2" "$1"
}
echo "== 1) 下載 SDL2 系列 MinGW devel =="
fetch "$base_sdl/SDL/releases/download/release-$SDL2_VER/SDL2-devel-$SDL2_VER-mingw.tar.gz"            SDL2.tgz
fetch "$base_sdl/SDL_image/releases/download/release-$IMG_VER/SDL2_image-devel-$IMG_VER-mingw.tar.gz"  SDL2_image.tgz
fetch "$base_sdl/SDL_ttf/releases/download/release-$TTF_VER/SDL2_ttf-devel-$TTF_VER-mingw.tar.gz"      SDL2_ttf.tgz
fetch "$base_sdl/SDL_mixer/releases/download/release-$MIX_VER/SDL2_mixer-devel-$MIX_VER-mingw.tar.gz"  SDL2_mixer.tgz

echo "== 2) 解壓並合併進單一 sysroot =="
for t in SDL2 SDL2_image SDL2_ttf SDL2_mixer; do
    rm -rf "$WORK/x_$t"; mkdir -p "$WORK/x_$t"
    tar xf "$WORK/dl/$t.tgz" -C "$WORK/x_$t"
    # 每包內含 <name>-<ver>/x86_64-w64-mingw32/{include,lib,bin}
    src=$(find "$WORK/x_$t" -maxdepth 2 -type d -name x86_64-w64-mingw32 | head -1)
    cp -r "$src/." "$SYS/"
done

echo "== 3) 重定位 .pc prefix 到 sysroot + CMake 交叉設定 =="
# SDL2 mingw 的 .pc prefix 是打包時的絕對路徑 → 改成我們的 sysroot
for pc in "$SYS"/lib/pkgconfig/*.pc; do
    sed -i "s|^prefix=.*|prefix=$SYS|" "$pc"
done
export PKG_CONFIG_LIBDIR="$SYS/lib/pkgconfig"
export PKG_CONFIG_PATH="$SYS/lib/pkgconfig"
rm -rf "$WORK/cmake"
cmake -S . -B "$WORK/cmake" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/packaging/mingw-toolchain.cmake" \
    -DMINGW_SDL_PREFIX="$SYS" \
    -DPKG_CONFIG_EXECUTABLE="$(command -v pkg-config)" \
    -DCMAKE_BUILD_TYPE=Release >/dev/null

echo "== 4) 編譯 =="
cmake --build "$WORK/cmake" -j"$(nproc)" >/dev/null
test -f "$WORK/cmake/u1_cht.exe"

echo "== 5) 組 zip(exe + DLL + 資產)=="
PKG="$WORK/Ultima1-CHT-windows-x64"; rm -rf "$PKG"; mkdir -p "$PKG"
cp "$WORK/cmake/u1_cht.exe" "$PKG/"
cp -r assets resources "$PKG/"
cp packaging/dist-config.json "$PKG/config.json"
cp packaging/README-dist.txt "$PKG/README.txt"
# 收 SDL DLL(devel 包的 bin/ 已含 image/ttf/mixer 各自的相依 DLL)
for t in SDL2 SDL2_image SDL2_ttf SDL2_mixer; do
    src=$(find "$WORK/x_$t" -maxdepth 2 -type d -name x86_64-w64-mingw32 | head -1)
    find "$src/bin" -name "*.dll" -exec cp -n {} "$PKG/" \;
done
# winpthread runtime DLL(libgcc/libstdc++ 已靜態連)
wp=$(find /usr -name libwinpthread-1.dll 2>/dev/null | head -1)
[ -n "$wp" ] && cp -n "$wp" "$PKG/"
# strip 符號:官方預編 SDL2_ttf.dll 帶完整 debug 符號達 65M(Mac 自編同庫僅 798K),
# strip 後大幅瘦身。exe/其他 DLL 一併去符號。
x86_64-w64-mingw32-strip --strip-unneeded "$PKG"/*.dll "$PKG"/*.exe 2>/dev/null || true
echo "  bundled DLLs:"; ls -1sh "$PKG"/*.dll | sed 's|/.*/|  |'

( cd "$WORK" && zip -qr "$OUT/Ultima1-CHT-windows-x64.zip" "$(basename "$PKG")" )
echo "完成 → $OUT/Ultima1-CHT-windows-x64.zip"
ls -la "$OUT/Ultima1-CHT-windows-x64.zip"
