#!/usr/bin/env bash
# 在 docker 容器內:xvfb 起 X → 跑遊戲 → N 秒後抓畫面 → 殺掉
# 用法(於 repo 根、容器內):  tools/shot.sh [輸出png] [等待秒數]
set -u
OUT="${1:-tests/snapshots/out/shot.png}"
WAIT="${2:-3}"
mkdir -p "$(dirname "$OUT")"

export DISPLAY=:99
Xvfb :99 -screen 0 1280x800x24 >/dev/null 2>&1 &
XPID=$!
sleep 1

./build/u1_cht > run.log 2>&1 &
GPID=$!
sleep "$WAIT"

# 抓整個 root window
import -window root "$OUT" 2>/dev/null || xwd -root -silent | convert xwd:- "$OUT" 2>/dev/null

kill -9 "$GPID" 2>/dev/null
kill -9 "$XPID" 2>/dev/null
wait 2>/dev/null

if [ -s "$OUT" ]; then
  echo "OK: $OUT ($(stat -c%s "$OUT") bytes)"
else
  echo "FAIL: 無截圖產出"; echo "--- run.log ---"; cat run.log
  exit 1
fi
