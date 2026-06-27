#!/usr/bin/env bash
# game tester:在 xvfb 下驅動「正常玩家路徑」(真實鍵序),每個動作後截圖。
# 須在容器內、repo 根執行。輸出 tests/snapshots/out/<step>.png 供 Read 目視判讀。
# 用法:tools/game_tester.sh
set -u
OUTDIR="tests/snapshots/out"
mkdir -p "$OUTDIR"

export DISPLAY=:99
export SDL_AUDIODRIVER=dummy   # headless 無真實音效裝置,用 dummy 讓音訊路徑可跑
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
key m;     shot t09_music_off      # M → 音樂:關
key m;     shot t10_music_on       # M → 音樂:開
key z;     shot t11_ztats          # Z → 角色屬性表
key space;                         # 關閉
key F10;   shot t12_quit_dialog    # F10 → 離開確認對話框(中文)
key Escape; shot t13_quit_cancel   # ESC → 取消,回遊戲(不可離開)

kill -9 "$GPID" 2>/dev/null; wait "$GPID" 2>/dev/null

# === RPG 系統煙霧(env-hook 直接進城鎮/城堡/地牢,驗模態不崩潰)===
rpg_smoke(){  # $1=env $2=開啟鍵 $3=label
  rm -f save.json
  env "$1=1" ./build/u1_cht > "run_$3.log" 2>&1 &
  local pid=$!; sleep 2.2
  local w=$(xdotool search --name "Ultima" 2>/dev/null | head -1)
  [ -n "$w" ] && xdotool windowactivate "$w" 2>/dev/null; sleep 0.3
  [ -n "$w" ] && xdotool key --window "$w" "$2" 2>/dev/null; sleep 0.4
  import -window root "$OUTDIR/rpg_$3.png" 2>/dev/null; echo "shot: rpg_$3"
  kill -9 "$pid" 2>/dev/null; wait "$pid" 2>/dev/null
  if grep -qiE "segfault|core dumped|Aborted|terminate" "run_$3.log"; then echo "RPG_SMOKE FAIL: $3"; else echo "rpg_smoke OK: $3"; fi
}
rpg_smoke U1_TEST_TOWN   b rpg_shop    # 城鎮 B 商店
rpg_smoke U1_TEST_CASTLE t rpg_king    # 城堡 T 國王
rpg_smoke U1_TEST_DUNGEON c rpg_cast   # 地牢 C 施法(空法術也不崩)
rm -f save.json

kill -9 "$XPID" 2>/dev/null; wait "$XPID" 2>/dev/null

echo "=== run.log (tail) ==="; tail -15 run.log
ls -la "$OUTDIR"/*.png 2>/dev/null | awk '{print $5, $9}'
