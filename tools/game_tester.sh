#!/usr/bin/env bash
# game tester:在 xvfb 下驅動「正常玩家路徑」(真實鍵序),每個動作後截圖。
# 須在容器內、repo 根執行。輸出 tests/snapshots/out/<step>.png 供 Read 目視判讀。
# 用法:tools/game_tester.sh
set -u
OUTDIR="tests/snapshots/out"
mkdir -p "$OUTDIR"

export DISPLAY=:99
Xvfb :99 -screen 0 1280x800x24 >/dev/null 2>&1 &
XPID=$!
sleep 1

./build/u1_cht > run.log 2>&1 &
GPID=$!
sleep 2.5   # 等載入地圖/字型

# 找遊戲視窗(SDL 視窗標題含 Ultima)
WID=$(xdotool search --name "Ultima" 2>/dev/null | head -1)
echo "window id: ${WID:-NOTFOUND}"
[ -n "$WID" ] && xdotool windowactivate "$WID" 2>/dev/null; sleep 0.5

shot(){ import -window root "$OUTDIR/$1.png" 2>/dev/null || xwd -root -silent | convert xwd:- "$OUTDIR/$1.png" 2>/dev/null; echo "shot: $1"; }
key(){ xdotool key --window "$WID" "$1" 2>/dev/null; sleep 0.6; }

# === 正常玩家路徑 ===
shot t01_start                     # 開場:世界地圖 + 中文狀態列
key Right; shot t02_move_right     # 移動 → 指令列應顯示「東」
key Down;  shot t03_move_down      # 「南」
key Left;  shot t04_move_left      # 「西」
key Up;    shot t05_move_up        # 「北」
key Up;    key Up; shot t06_move_more
key Next;  shot t07_tileset_toggle # PageDown 切 EGA/CGA tileset
key e;     shot t08_enter          # 嘗試進入(視位置而定)
key F10;   shot t09_quit_dialog    # F10 → 離開確認對話框(中文)
key Escape; shot t10_quit_cancel   # ESC → 取消,回遊戲(不可離開)

kill -9 "$GPID" 2>/dev/null
kill -9 "$XPID" 2>/dev/null
wait 2>/dev/null

echo "=== run.log (tail) ==="; tail -15 run.log
ls -la "$OUTDIR"/t0*.png 2>/dev/null | awk '{print $5, $9}'
