#!/usr/bin/env bash
# 做「完整封存包」(含版權原版 gamedata,解壓即玩)——個人保存用,非公開 release。
# 把 dist/release/ 的 CI artifact + 本機 gamedata/ 注入,輸出到 dist/complete/。
# 用法:先 gh run download <run> -D dist/release,放好 gamedata/,再:
#   bash packaging/make_complete_backup.sh
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
REL="$ROOT/dist/release"; GD="$ROOT/gamedata"; OUT="$ROOT/dist/complete"
mkdir -p "$OUT"
[ -d "$GD" ] || { echo "缺 gamedata/(版權原版 BIN),放好再跑"; exit 1; }
WT="$(mktemp -d)"

# Windows:zip 解開 → 塞 gamedata → 重 zip
WZ=$(find "$REL" -name "*windows*x64.zip" | head -1)
if [ -n "$WZ" ]; then
  rm -rf "$WT/w"; mkdir -p "$WT/w"; ( cd "$WT/w" && unzip -q "$WZ" )
  D=$(find "$WT/w" -maxdepth 1 -mindepth 1 -type d | head -1)
  cp -r "$GD" "$D/gamedata"
  rm -f "$OUT/Ultima1-CHT-windows-x64-COMPLETE.zip"
  ( cd "$WT/w" && zip -qr "$OUT/Ultima1-CHT-windows-x64-COMPLETE.zip" "$(basename "$D")" )
  echo "✓ Windows → Ultima1-CHT-windows-x64-COMPLETE.zip"
fi

# Linux AppImage:唯讀,不可注入 → AppImage + gamedata/ 同層打 tar.gz(程式以啟動目錄找 gamedata)
AI=$(find "$REL" -name "*.AppImage" | head -1)
if [ -n "$AI" ]; then
  rm -rf "$WT/l"; mkdir -p "$WT/l/Ultima1-CHT-linux"
  cp "$AI" "$WT/l/Ultima1-CHT-linux/"; cp -r "$GD" "$WT/l/Ultima1-CHT-linux/gamedata"
  printf '解壓後執行:  ./%s\n(gamedata 已同附,存檔在 ~/.local/share/LCY/Ultima1-CHT/)\n' "$(basename "$AI")" \
    > "$WT/l/Ultima1-CHT-linux/執行.txt"
  ( cd "$WT/l" && tar czf "$OUT/Ultima1-CHT-linux-COMPLETE.tar.gz" Ultima1-CHT-linux )
  echo "✓ Linux → Ultima1-CHT-linux-COMPLETE.tar.gz"
fi

# macOS:.app zip 解開 → gamedata 塞進 Contents/MacOS/gamedata → 重 zip(每架構各一)
for MZ in $(find "$REL" -name "*macos*.zip" 2>/dev/null); do
  arch=$(echo "$MZ" | grep -oE "arm64|x86_64" | head -1)
  rm -rf "$WT/m"; mkdir -p "$WT/m"; ( cd "$WT/m" && unzip -q "$MZ" )
  APP=$(find "$WT/m" -maxdepth 1 -name "*.app" | head -1)
  [ -n "$APP" ] && cp -r "$GD" "$APP/Contents/MacOS/gamedata"
  rm -f "$OUT/Ultima1-CHT-macos-${arch}-COMPLETE.zip"
  ( cd "$WT/m" && zip -qry "$OUT/Ultima1-CHT-macos-${arch}-COMPLETE.zip" "$(basename "$APP")" )
  echo "✓ macOS $arch → Ultima1-CHT-macos-${arch}-COMPLETE.zip"
done

rm -rf "$WT"
echo "=== 完整包 ==="; ls -lh "$OUT/"
