#!/usr/bin/env bash
# 建 Linux AppImage:Release build → AppDir(binary + 資產同層)→ linuxdeploy 收 .so → appimagetool。
# 於 u1-cht docker 容器內、repo 根執行(容器需 wget/FUSE-less):
#   docker run --rm -v "$PWD":/work -w /work u1-cht bash packaging/build-appimage.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
OUT="$ROOT/dist"
APPDIR="$ROOT/build-appimage/AppDir"
TOOLS="$ROOT/build-appimage/tools"
ARCH="$(uname -m)"
mkdir -p "$OUT" "$TOOLS"
rm -rf "$APPDIR"; mkdir -p "$APPDIR/usr/bin"

echo "== 1) Release build =="
cmake -S . -B build-appimage/cmake -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build-appimage/cmake -j"$(nproc)" >/dev/null

echo "== 2) 組 AppDir(binary 與資產同層,配合 chdir(SDL_GetBasePath)) =="
cp build-appimage/cmake/u1_cht "$APPDIR/usr/bin/"
cp -r assets resources "$APPDIR/usr/bin/"
cp packaging/dist-config.json "$APPDIR/usr/bin/config.json"
# desktop + icon(AppImage 規格要求在 AppDir 根)
cp packaging/u1_cht.desktop "$APPDIR/u1_cht.desktop"
cp packaging/icon.png "$APPDIR/u1_cht.png"
# AppRun:直接 exec binary(路徑穩健性由程式內 chdir 處理)
cat > "$APPDIR/AppRun" <<'RUN'
#!/bin/bash
HERE="$(dirname "$(readlink -f "${0}")")"
exec "${HERE}/usr/bin/u1_cht" "$@"
RUN
chmod +x "$APPDIR/AppRun"

echo "== 3) 取 linuxdeploy / appimagetool =="
dl() { [ -f "$2" ] || wget -qO "$2" "$1"; chmod +x "$2"; }
dl "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage" "$TOOLS/linuxdeploy"
dl "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-${ARCH}.AppImage" "$TOOLS/appimagetool"

echo "== 4) 收 .so 依賴 =="
export NO_STRIP=1 APPIMAGE_EXTRACT_AND_RUN=1
"$TOOLS/linuxdeploy" --appimage-extract-and-run \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/u1_cht" \
    --desktop-file "$APPDIR/u1_cht.desktop" \
    --icon-file "$APPDIR/u1_cht.png" >/dev/null

echo "== 5) 打包 AppImage =="
ARCH="$ARCH" "$TOOLS/appimagetool" --appimage-extract-and-run "$APPDIR" "$OUT/Ultima1-CHT-${ARCH}.AppImage" >/dev/null
cp packaging/README-dist.txt "$OUT/README.txt"
echo "完成 → $OUT/Ultima1-CHT-${ARCH}.AppImage"
ls -la "$OUT/"
