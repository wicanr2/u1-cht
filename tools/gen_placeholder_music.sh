#!/usr/bin/env bash
# 產生占位背景音樂(程序生成,無版權):簡單五聲音階琶音循環,柔和淡入淡出。
# FM Towns 真音樂之後再替換。須在容器內(有 ffmpeg)執行。
set -e
OUT="assets/music/theme.ogg"
mkdir -p assets/music tmp_notes

# 五聲音階上行+下行(Hz),每音 0.45s
NOTES=(220.00 261.63 293.66 329.63 392.00 440.00 392.00 329.63 293.66 261.63)
i=0
LIST=""
for f in "${NOTES[@]}"; do
  ffmpeg -y -f lavfi -i "sine=frequency=${f}:duration=0.45" \
    -af "afade=t=in:st=0:d=0.05,afade=t=out:st=0.4:d=0.05,volume=0.35" \
    "tmp_notes/n${i}.wav" >/dev/null 2>&1
  LIST="${LIST}file 'n${i}.wav'\n"
  i=$((i+1))
done

printf "$LIST" > tmp_notes/list.txt
# 串接一輪 → ogg(遊戲端 -1 無限循環)
ffmpeg -y -f concat -safe 0 -i tmp_notes/list.txt -c:a libvorbis -q:a 3 "$OUT" >/dev/null 2>&1
rm -rf tmp_notes
echo "generated $OUT ($(stat -c%s "$OUT") bytes)"
