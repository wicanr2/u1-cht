#!/usr/bin/env bash
# 創世紀一代繁中 remake — 推廣短片合成(照 docs/promo-video-style.md 的創世紀古卷風格)。
# 投影片(實機截圖)+ Ken Burns + 鎏金襯線標題 + 羊皮紙引子 + 安卡結尾 + 遊戲音樂。
# 於 u1-cht docker 容器內、repo 根執行(需 ffmpeg / imagemagick / Noto Serif CJK):
#   docker run --rm -v "$PWD":/work -w /work u1-cht bash packaging/make_promo_video.sh
set -u
cd "$(dirname "$0")/.."
SHOT=docs/img
OUT=dist/video; mkdir -p "$OUT"
TMP="$(mktemp -d)"
W=1280; H=720; FPS=25

# ===== 設計 token(創世紀古卷風,換皮只改這裡)=====
BG='#0d1b2a'; BGDEEP='#070d14'
GOLD='#d4a017'; GOLDHI='#f0c869'; GOLDSH='#8a6d1a'
PARCH='#e8d8b0'; PARCHDK='#c9b083'
RED='#7c1f12'; CREAM='#f4ecd8'
FB=/usr/share/fonts/opentype/noto/NotoSerifCJK-Bold.ttc      # 襯線粗(標題)
FR=/usr/share/fonts/opentype/noto/NotoSerifCJK-Regular.ttc   # 襯線(字幕)
MUSIC=assets/music/fmtowns.ogg                                # 遊戲自製 BGM(對味、無版權)
[ -f "$MUSIC" ] || MUSIC=assets/music/theme.ogg

# 星空底(深夜藍 + 疏散星點 + 暗角)
starfield(){  # $1 out
  convert -size ${W}x${H} xc:"$BG" \
    \( -size ${W}x${H} xc:black -fill white -draw "$(for i in $(seq 1 90); do echo "point $((RANDOM%W)),$((RANDOM%H))"; done)" -blur 0x0.4 \) \
    -compose screen -composite \
    \( -size ${W}x${H} radial-gradient:none-"$BGDEEP" \) -compose over -composite "$1"
}

# 鎏金浮雕文字(陰影+主金+高光三層)疊到既有圖上
emboss(){  # $1 in/out  $2 font  $3 size  $4 text  $5 yoff
  convert "$1" -font "$2" -pointsize "$3" -gravity center \
    -fill "$GOLDSH" -annotate +3+$(($5+3)) "$4" \
    -fill "$GOLD"   -annotate +0+$5        "$4" \
    -fill "$GOLDHI" -annotate +0+$(($5-2)) "$4" "$1"
}

# 安卡 ☥(程序畫:上橢圓環 + 十字)金色
ankh(){  # $1 out  $2 cx  $3 cy  $4 scale
  local s=$4 cx=$2 cy=$3
  convert -size ${W}x${H} xc:none -stroke "$GOLD" -strokewidth $((s/6)) -fill none \
    -draw "ellipse $cx,$((cy-s)) $((s*3/5)),$s 0,360" \
    -draw "line $cx,$((cy-s/3)) $cx,$((cy+s*2))" \
    -draw "line $((cx-s)),$((cy+s/2)) $((cx+s)),$((cy+s/2))" \
    \( +clone -fill "$GOLDHI" -colorize 0 -blur 0x6 \) -compose screen -composite "$1"
}

# 標題卡:星空 + 鎏金大標 + 紅副標 + 安卡 + 金框
titlecard(){  # $1 out
  starfield "$TMP/_bg.png"
  ankh "$TMP/_ankh.png" $((W/2)) 150 26
  convert "$TMP/_bg.png" "$TMP/_ankh.png" -compose over -composite "$1"
  emboss "$1" "$FB" 96 "創世紀一代" -30
  convert "$1" -font "$FB" -pointsize 52 -gravity center -fill "$RED" -annotate +0+70 "黑 暗 紀 元" "$1"
  convert "$1" -font "$FR" -pointsize 30 -gravity center -fill "$CREAM" -annotate +0+150 "Ultima I: The First Age of Darkness　·　繁體中文版" "$1"
  # 金框
  convert "$1" -fill none -stroke "$GOLD" -strokewidth 3 -draw "roundrectangle 28,28 $((W-28)),$((H-28)) 16,16" \
    -stroke "$GOLDSH" -strokewidth 1 -draw "roundrectangle 36,36 $((W-36)),$((H-36)) 14,14" "$1"
}

# 結尾卡:漸暗 + 大安卡 + 標題 + repo
endcard(){  # $1 out
  convert -size ${W}x${H} xc:"$BGDEEP" "$1"
  ankh "$TMP/_ankh2.png" $((W/2)) 220 40
  convert "$1" "$TMP/_ankh2.png" -compose over -composite "$1"
  emboss "$1" "$FB" 64 "創世紀一代:黑暗紀元" 110
  convert "$1" -font "$FR" -pointsize 34 -gravity center -fill "$CREAM" -annotate +0+185 "繁體中文版　·　SDL2 復刻　·　免費開源" "$1"
  convert "$1" -font "$FR" -pointsize 26 -gravity center -fill "$PARCHDK" -annotate +0+250 "github.com/wicanr2/u1-cht" "$1"
}

# 羊皮紙引子卡(有機紙紋 + 做舊暗角 + 史詩字幕)
parchcard(){  # $1 out  $2 line1  $3 line2  $4 line3
  # 淺羊皮紙底 + 適度可見的纖維噪聲(multiply 壓在亮底上)+ 輕做舊暗角
  convert -size ${W}x${H} xc:"$PARCH" \
    \( -size ${W}x${H} xc: +noise Gaussian -colorspace Gray -level 35%,66% -blur 0x0.5 \) \
    -compose multiply -composite \
    \( -size ${W}x${H} xc: +noise Impulse -colorspace Gray -level 80%,100% \) \
    -compose multiply -composite \
    -fill "$PARCHDK" -colorize 7% \
    \( +clone -fill black -colorize 100% -fill white \
       -draw "roundrectangle 44,44 $((W-44)),$((H-44)) 22,22" -blur 0x55 -level 0%,72% \) \
    -compose multiply -composite "$1"
  convert "$1" -font "$FR" -gravity center -fill '#3a2c0e' -pointsize 42 \
    -annotate +0-70 "$2" -annotate +0+0 "$3" -annotate +0+70 "$4" "$1"
  convert "$1" -fill none -stroke "$GOLDSH" -strokewidth 2 -draw "roundrectangle 36,36 $((W-36)),$((H-36)) 14,14" "$1"
}

# 亮點投影片:星空底 + 截圖(金框)+ 鎏金字幕
slide(){  # $1 out  $2 screenshot  $3 caption
  starfield "$TMP/_sb.png"
  convert "$TMP/_sb.png" \
    \( "$SHOT/$2" -resize ${W}x500\> -bordercolor "$GOLD" -border 3 \) \
    -gravity north -geometry +0+50 -compose over -composite "$1"
  emboss "$1" "$FB" 44 "$3" 300
  convert "$1" -fill none -stroke "$GOLDSH" -strokewidth 2 -draw "roundrectangle 28,28 $((W-28)),$((H-28)) 12,12" "$1"
}

# 七平台美術拼貼(2x4 格)
montage7(){  # $1 out
  starfield "$TMP/_mb.png"
  montage "$SHOT/screen_ega.png" "$SHOT/screen_cga.png" "$SHOT/screen_fmtowns.png" "$SHOT/screen_msx.png" \
          "$SHOT/screen_pc98.png" "$SHOT/screen_iigs.png" "$SHOT/screen_atari.png" "$SHOT/screen_vga.png" \
          -tile 4x2 -geometry 300x188+5+5 -bordercolor "$GOLDSH" -border 1 -background none "$TMP/_grid.png"
  convert "$TMP/_mb.png" \( "$TMP/_grid.png" -resize $((W*92/100))x560\> \) \
    -gravity center -geometry +0-26 -compose over -composite "$1"
  emboss "$1" "$FB" 40 "七平台美術　橫跨四十年的經典重生" 312
  convert "$1" -fill none -stroke "$GOLDSH" -strokewidth 2 -draw "roundrectangle 28,28 $((W-28)),$((H-28)) 12,12" "$1"
}

# 靜態投影片 + 淡入淡出(loop 圖即可,秒出;zoompan 逐幀縮放太慢,捨棄)。
kb(){  # $1 png  $2 mp4  $3 dur
  local FO; FO=$(awk "BEGIN{print $3-0.5}")
  ffmpeg -y -loglevel error -threads 0 -loop 1 -t "$3" -i "$1" \
    -vf "fps=$FPS,format=yuv420p,fade=t=in:st=0:d=0.5,fade=t=out:st=$FO:d=0.5" \
    -c:v libx264 -preset veryfast -pix_fmt yuv420p "$2"
}

echo "== 烘卡片 =="
titlecard "$TMP/00.png"
parchcard "$TMP/01.png" "邪術師蒙登,以不死寶石奴役薩薩瑞亞。" "汝乃異界召喚而來的『陌生人』——" "從最深的地牢,到最遠的星海。"
slide "$TMP/02.png" screen_pkg_paths.png   "踏遍薩薩瑞亞的四方疆土"
slide "$TMP/03.png" screen_charcreate.png  "選定種族與職業,打造汝之英雄"
slide "$TMP/04.png" screen_vehicles.png    "於城鎮購置兵刃、法術與坐騎"
slide "$TMP/05.png" screen_dungeon_lv5.png "深入不見天日的地底迷宮"
slide "$TMP/06.png" screen_dungeon_combat.png "以劍與魔法迎戰蒙登的爪牙"
slide "$TMP/07.png" screen_space.png       "駕太空梭,於繁星之海纏鬥"
montage7 "$TMP/08.png"
slide "$TMP/09.png" screen_endgame_king.png "啟動時光機,回到黑暗未臨之時"
endcard "$TMP/99.png"

echo "== 卡片轉片段(Ken Burns)=="
LIST="$TMP/list.txt"; : > "$LIST"
# 段落總長刻意 ≈48s(略短於 fmtowns.ogg 49.9s)→ 音樂只播一次、不必 loop(loop 接縫是「怪」的主因)
for spec in 00:4.5 01:5.0 02:4.0 03:4.0 04:4.0 05:4.0 06:4.0 07:4.5 08:4.5 09:4.0 99:5.5; do
  f=${spec%%:*}; d=${spec##*:}
  kb "$TMP/$f.png" "$TMP/seg_$f.mp4" "$d"
  echo "file '$TMP/seg_$f.mp4'" >> "$LIST"
done

echo "== concat =="
ffmpeg -y -loglevel error -f concat -safe 0 -i "$LIST" -c:v libx264 -pix_fmt yuv420p -crf 19 "$TMP/silent.mp4"
DUR=$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$TMP/silent.mp4")

MDUR=$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$MUSIC")
echo "== 鋪 FM Towns 遊戲音樂(只播一次,不 loop;影片 ${DUR}s ≤ 音樂 ${MDUR}s)=="
FO=$(awk "BEGIN{print $DUR-3.5}")
ffmpeg -y -loglevel error -i "$MUSIC" -i "$TMP/silent.mp4" \
  -filter_complex "[0:a]atrim=0:$DUR,afade=t=in:st=0:d=2.5,afade=t=out:st=$FO:d=3.5,volume=0.9[a]" \
  -map 1:v -map "[a]" -c:v copy -c:a aac -b:a 192k -shortest -movflags +faststart \
  "$OUT/u1cht-promo.mp4"
rm -rf "$TMP"
echo "→ $OUT/u1cht-promo.mp4 ($(du -h "$OUT/u1cht-promo.mp4" | cut -f1), ${DUR}s)"
