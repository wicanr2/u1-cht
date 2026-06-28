# 用 LLM 建立遊戲推廣影片 — Pipeline 教學

> 給「老遊戲繁中化 / remake」專案的可重用方法論:**不開剪輯軟體,用腳本 + LLM 把實機畫面合成推廣短片**。
> 做一次流水線,每款遊戲重用(換皮 + 換內容即可)。本文以 Indiana Jones 繁中 / 創世紀一代 remake 為例。

---

## 0. TL;DR

- **Pipeline = 三段可腳本化的生產線**:① 擷取實機畫面+音樂 → ② 備素材(截圖/卡片/底紋)→ ③ ffmpeg 合成(投影片+字幕+轉場+配樂)。
- **為什麼不用剪輯軟體**:腳本化 = 可重跑、可進版控、可被 LLM 驅動、換遊戲只改參數。手剪一次性、不可重現。
- **LLM 的角色**:研究風格 → 生擷取腳本(按鍵編排)→ 生合成腳本(ffmpeg 指令)→ 寫文案 → **讀算繪出的 frame 迭代**。
- **「對接/換皮」概念**:同一套合成引擎,把「色票 + 字體 + 底紋 + 母題 + 轉場」這組**設計 token** 抽出來;換一組就換一個風格(Indy 深藍 → 創世紀古卷)。引擎不動。

---

## 1. 什麼是「影片 pipeline」

把「做一支介紹片」拆成三個**各自獨立、可重跑**的階段。每階段都是一支 shell 腳本,輸入/輸出是檔案。

```
┌─ ① 擷取 capture ─┐   ┌─ ② 素材 assets ─┐   ┌─ ③ 合成 compose ─┐
│ Xvfb 無頭跑遊戲  │   │ 遊戲截圖(PNG)   │   │ 投影片 + Ken Burns│
│ ffmpeg x11grab   │──▶│ 標題/結尾卡      │──▶│ 中文字幕 + 轉場   │──▶ final.mp4
│ SDL disk 錄音樂  │   │ 羊皮紙/底紋素材  │   │ 鋪遊戲音樂 + 淡入淡出│
│ xdotool 送按鍵   │   │ (程序生成)      │   │ (ffmpeg/ImageMagick)│
└──────────────────┘   └──────────────────┘   └───────────────────┘
   產 cap.mp4/cap.wav      產 *.png               產 intro.mp4
```

### 為什麼這樣分

- **可重現**:改一句字幕、換一張截圖,只重跑第 ③ 段,不必重錄。
- **無頭可 CI**:第 ① 段用 `Xvfb`(虛擬螢幕)→ 伺服器/CI 也能錄,不需真螢幕。
- **無版權音樂問題**:配樂直接錄**遊戲自己的實機音樂**(SDL `disk` 音訊後端把聲音寫成檔)。
- **LLM 可全程驅動**:每段都是文字腳本,LLM 能生成、能改、能讀輸出 frame 判斷對不對。

---

## 2. LLM 在每一段做什麼

| 階段 | LLM 的工作 | 產出 |
|---|---|---|
| **0 風格研究** | 上網查該遊戲/系列的視覺 DNA(封面、字體、配色、母題、氛圍),整理成**風格指南 markdown**(設計 token 要具體:hex、字體名、母題清單)| `style.md` |
| **① 擷取** | 寫**按鍵編排腳本**:用 `xdotool` 在對的時間點送鍵,把遊戲開到要錄的畫面(標題→過場→實機操作)。時間軸要對齊 ffmpeg 的 0 秒 | `capture.sh` |
| **② 素材** | 生**程序化素材指令**(ImageMagick 做羊皮紙底紋、金色浮雕標題、裝飾邊框);列出要用哪些截圖 | 素材 + 截圖清單 |
| **③ 合成** | 寫 **ffmpeg 合成腳本**(投影片 Ken Burns、`drawtext` 字幕、`xfade` 轉場、音樂鋪底淡入淡出、concat)| `make.sh` |
| **文案** | 寫**對味的中文字幕**(史詩古卷 / 冒險 / 幽默,依風格指南的語氣)| 字幕字串 |
| **迭代** | **讀算繪出的 frame / 成片截圖**,判斷:標題糊不糊、字幕有沒有被裁、配色對不對、節奏會不會太快 → 改參數重跑 | 修正 |

> 關鍵:LLM 能**看圖**。算繪幾幀出來丟給 LLM 看,它能指出「金字太亮像螢光黃,改 #d4a017」「字幕第二行被裁,縮 fontsize」——這是純文字 LLM 做不到、但對影片品質決定性的迴圈。

---

## 3. 可重用骨架(兩支腳本)

### ① `capture_gameplay_video.sh` — 無頭擷取

核心三件事:**Xvfb 開虛擬螢幕 → 背景送按鍵驅動 → ffmpeg 同時錄畫面;SDL disk 後端錄音樂**。

```bash
#!/usr/bin/env bash
set -u
SECS="${1:-42}"; DISP=:96; WH=960x720
RAW=/tmp/cap.raw; VID=/tmp/cap.mp4; WAV=/tmp/cap.wav
Xvfb $DISP -screen 0 ${WH}x24 >/dev/null 2>&1 & XP=$!; sleep 1

# SDL disk 音訊後端:把遊戲聲音寫成 raw 檔(無需音效卡)
DISPLAY=$DISP SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE="$RAW" \
  ./your_game >/dev/null 2>&1 & GP=$!; sleep 1

# 背景按鍵編排(時間軸對齊 ffmpeg 0s)— LLM 依遊戲流程生成這段
( K(){ DISPLAY=$DISP xdotool key "$1"; }
  sleep 3;  K Return                 # 關開場 modal
  sleep 2;  for d in Right Right Down; do K $d; sleep 0.8; done  # 走幾步
  sleep 4;  K e                      # 進城鎮…
) & KP=$!

# 同時錄影(固定秒數,有界)
DISPLAY=$DISP ffmpeg -y -f x11grab -video_size $WH -framerate 25 -i $DISP \
  -t "$SECS" -c:v libx264 -pix_fmt yuv420p -crf 18 "$VID"
kill $KP $GP $XP 2>/dev/null
# raw PCM → wav
ffmpeg -y -f s16le -ar 44100 -ac 2 -i "$RAW" "$WAV"
```

要點:**固定秒數 SIGKILL 收尾**(有界、CI 友善),不靠 GUI、不等 sentinel。

### ② `make_gameplay_video.sh` — ffmpeg 合成

把**設計 token 放最上面**(這就是「換皮」的開關),下面是通用函式 + 分鏡。

```bash
#!/usr/bin/env bash
set -u
# ===== 設計 token(換皮只改這裡)=====
BG='#0d1b2a'; GOLD='#d4a017'; GOLD_HI='#f0c869'; CAP='#f4ecd8'   # 創世紀古卷
FONT=/usr/share/fonts/.../NotoSerifCJKtc-Black.otf               # 襯線(非黑體!)
W=1280; H=720; FPS=25
SHOT=screenshots; CAP_V=/tmp/cap.mp4; CAP_A=/tmp/cap.wav
TMP=$(mktemp -d); OUT=dist/video; mkdir -p "$OUT"

# ----- 通用函式 -----
card(){ convert -size ${W}x${H} xc:"$BG" -font "$FONT" -gravity center \
  -fill "$GOLD" -pointsize 64 -annotate +0-40 "$2" \
  -fill "$CAP"  -pointsize 32 -annotate +0+60 "$3" "$1"; }     # 標題/結尾卡
slide(){ convert -size ${W}x${H} xc:"$BG" \
  \( "$SHOT/$2" -resize x560 -bordercolor "$GOLD" -border 2 \) \
  -gravity north -geometry +0+30 -composite \
  -font "$FONT" -fill "$CAP" -gravity south -pointsize 36 -annotate +0+45 "$3" "$1"; }
kenburns(){ local FO; FO=$(awk "BEGIN{print $3-0.5}")
  ffmpeg -y -loglevel error -loop 1 -t "$3" -i "$1" \
    -vf "fps=$FPS,format=yuv420p,fade=t=in:st=0:d=0.5,fade=t=out:st=$FO:d=0.5,\
         zoompan=z='min(zoom+0.0008,1.08)':d=$(($FPS*${3%.*})):s=${W}x${H}" \
    -c:v libx264 -pix_fmt yuv420p "$2"; }                       # Ken Burns 緩推

# ----- 分鏡(LLM 依風格指南填內容/字幕)-----
card  "$TMP/00.png" '創世紀一代' '黑暗紀元　繁體中文版 · SDL2 復刻'
slide "$TMP/01.png" world.png   '踏遍薩薩瑞亞的四方疆土'
slide "$TMP/02.png" dungeon.png '深入不見天日的地底迷宮'
slide "$TMP/03.png" space.png   '駕太空梭,於繁星之海纏鬥'
card  "$TMP/99.png" '☥' '回到黑暗未臨之時 · 免費開源'

# ----- 卡片轉片段 + concat + 鋪音樂(淡入淡出)-----
LIST="$TMP/list.txt"; : > "$LIST"
for f in 00 01 02 03 99; do
  D=4; kenburns "$TMP/$f.png" "$TMP/s_$f.mp4" "$D"
  echo "file '$TMP/s_$f.mp4'" >> "$LIST"
done
ffmpeg -y -f concat -safe 0 -i "$LIST" -c:v libx264 -pix_fmt yuv420p "$TMP/silent.mp4"
DUR=$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$TMP/silent.mp4")
FO=$(awk "BEGIN{print $DUR-3}")
ffmpeg -y -i "$TMP/silent.mp4" -i "$CAP_A" \
  -filter_complex "[1:a]atrim=0:$DUR,afade=t=in:st=0:d=2,afade=t=out:st=$FO:d=3[a]" \
  -map 0:v -map "[a]" -c:v libx264 -c:a aac -b:a 192k -shortest -movflags +faststart \
  "$OUT/intro.mp4"
echo "→ $OUT/intro.mp4"
```

---

## 4. 「對接表 / 換皮」是什麼意思

風格指南末尾那張表(`promo-video-style.md` 的 H 節),意思是:**同一支 `make.sh`,把最上面那組設計 token 換掉,就從一個遊戲的風格變成另一個**。引擎(函式 + 分鏡結構)完全不動。

| 換皮的開關(token) | Indy 冒險風 | 創世紀古卷風 |
|---|---|---|
| 背景 `BG` | `#0a0e1a` 純深藍黑 | `#0d1b2a` 深夜星空 + vignette |
| 標題金 `GOLD` | `#f0c000` 明亮黃 | `#d4a017` 鎏金 + 三層浮雕 |
| `FONT` | Noto **Sans** CJK Black | Noto **Serif** CJK + Trajan/符文 |
| 底卡 | 純色 | 程序生成羊皮紙底紋 |
| 母題/轉場 | 一般 fade | 安卡☥ / 月之門光環 / 金裝飾角 |

> **這就是 pipeline 的價值**:第一款遊戲花力氣把流水線 + 函式寫好;之後每款只做兩件事 ——
> ① LLM 研究出該遊戲的**設計 token**(換皮),② LLM 依劇情寫**分鏡 + 字幕**(換內容)。**省下 90% 的重工**。

---

## 5. 實作走一遍(以 u1-cht 為例)

1. **風格**:LLM 上網研究 → `docs/promo-video-style.md`(色票/字體/母題/分鏡)。
2. **能截圖**:確認遊戲能在 docker + Xvfb 跑、能截圖(本專案有 `tools/game_tester.sh` 的截圖機制可複用)。
3. **擷取**:寫 `capture.sh`,xdotool 把遊戲開到「世界圖→城鎮→地牢→星海」並錄影錄樂。
4. **素材**:IM 生羊皮紙底 + 鎏金標題;從擷取/game_tester 撈各亮點截圖(7 平台 tileset、地牢、戰鬥、星海、勝利)。
5. **合成**:`make.sh` 套創世紀 token + 分鏡(段 E 那 7 個亮點)。
6. **迭代**:算 3–4 幀 → LLM 讀圖 → 修色/字幕/節奏 → 重跑第 ⑤ 步。
7. **產出**:`dist/video/u1cht-intro.mp4`,放 README / YouTube。

---

## 6. 換一款遊戲怎麼重用(for future promotion)

把 `capture.sh` + `make.sh` 當**模板**抄到新專案,然後:

1. LLM 研究新遊戲的視覺風格 → 新 `style.md` + 新一組 token(換 `make.sh` 開頭 5 行)。
2. LLM 依新遊戲流程重寫 `capture.sh` 的**按鍵編排**(每個遊戲開到指定畫面的鍵序不同)。
3. LLM 依新遊戲劇情重寫**分鏡 + 字幕**。
4. 算繪 → 看圖 → 迭代。

引擎(Xvfb 擷取機制、Ken Burns 函式、concat/配樂流程)**完全不改**。這就是為什麼值得把它寫成 pipeline 而非手剪。

---

## 7. 地雷與 tips

- **時間軸對齊**:`capture.sh` 的按鍵 `sleep` 要對齊 ffmpeg 的 0 秒(ffmpeg 一起跑時才開始計);抓「實機 logo/標題」片段要算好 `-ss` 起點。
- **字幕被裁**:中文全形寬,`fontsize` 太大會超出;LLM **看成片截圖**確認不被裁。
- **金字別螢光**:純 `#ffff00`/`#f0c000` 數位感重;要「做舊金 + 浮雕」(三層 drawtext 偏移)。
- **截圖保原色**:像素遊戲截圖**不要調色**,保 EGA/原始盤;只在外面加框,對比才對。
- **節奏**:莊嚴風前段慢(2–4s/幀)、亮點段卡音樂節拍(1.5–2.5s);Ken Burns 幅度小、勻速,忌快速甩鏡。
- **有界執行**:擷取一定固定秒數收尾(SIGKILL),別等 GUI / sentinel(CI/無頭友善)。
- **音樂用遊戲自己的**:`SDL_AUDIODRIVER=disk` 錄實機音樂,零版權風險、最對味。
- **全程可 CI**:三支腳本都無頭,可放 GitHub Actions 自動產片(同打包流程)。

---

## 附:相關檔案
- 風格指南:[`docs/promo-video-style.md`](promo-video-style.md)
- 參考 pipeline 實作:`/home/anr2/cht/indiana-jones-and-last-crusade-cht/scripts/{capture,make}_gameplay_video.sh`
- 截圖機制可複用:`tools/game_tester.sh`
