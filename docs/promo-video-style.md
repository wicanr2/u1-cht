# 《創世紀繁中推廣影片 — 視覺風格指南》

> 研究日期:2026-06-28　|　對象專案:《創世紀一代:黑暗紀元》SDL2 繁中 remake　|　用途:60–90 秒 YouTube 推廣短片(ffmpeg 合成)
> pipeline 參考:`/home/anr2/cht/indiana-jones-and-last-crusade-cht/scripts/{capture,make}_gameplay_video.sh`(深藍底+金字+Ken Burns+遊戲音樂的 ffmpeg 合成骨架)。

---

## A. 風格一句話定義

**「一本被翻開的中世紀魔法古卷,卷頁裡映出 1981 年 EGA 像素的薩薩瑞亞(Sosaria),金色襯線標題如紋章浮現,而扉頁角落悄悄藏著太空梭與時光機。」**

拆解這句話的三個必要成分,缺一就不是「創世紀感」而只是通用奇幻:

1. **古卷實體感(parchment realism)**——創世紀從一代起就不只是「一張遊戲」,而是「一個盒裝世界」:布質地圖(cloth map)、羊皮紙手冊、金屬安卡(ankh)護身符、符文路標。視覺基調是「攤在桌上的實體文物」,不是螢光奇幻海報。
2. **Denis Loubet 的奇幻寫實油畫**——同一位畫師從 Ultima I(1981)畫到 Ultima IX,定義了整個西方 CRPG 子類型的氣質:傳統媒材(鋼筆、油彩)、暗調、騎士對龍、莊嚴而非熱血。**暗、厚塗、史詩、寫實光影**,不是日系明亮、不是卡通描邊。
3. **U1 獨有的「奇幻混科幻」**——標語就是 *"From darkest dungeons to deepest space!"*(從最深地牢到最遠太空)。一代有太空梭坐騎、星海太空戰、時光機回到過去殺蒙登(Mondain)。這條反差是 U1 區別於後續所有創世紀的簽名,**影片必須吃到這一口**,否則做成了 U4/U5 的莊嚴聖賢風,丟了一代的野性。

一句話的張力公式:**莊嚴古卷(80%)+ 一閃科幻違和(20%)**。

---

## B. 配色票(設計 token,直接可用)

創世紀的色感來自三個疊層:**羊皮紙文物層** + **EGA/CGA 像素遊戲層** + **深藍夜空史詩層**。給出可直接寫進 ffmpeg/ImageMagick 的 hex:

### 主色票(影片框架用)

| Token | Hex | 用途 |
|---|---|---|
| `--ink-night` 深墨夜藍 | `#0d1b2a` | 主背景底(比 Indiana 的 `#0a0e1a` 略帶藍綠,更像深夜星空而非純黑) |
| `--ink-deep` 近黑藍 | `#070d14` | 暗角 vignette、結尾卡漸暗 |
| `--gold-title` 紋章金 | `#d4a017` | 標題主金(比 Indiana 的 `#f0c000` 暗、更舊、更「古銅鎏金」而非「明亮黃」) |
| `--gold-bright` 高光金 | `#f0c869` | 金字描邊內側高光、符文浮現的發光 |
| `--gold-shadow` 暗金 | `#8a6d1a` | 金字外緣陰影、做出浮雕厚度 |
| `--parchment` 羊皮紙米黃 | `#e8d8b0` | 字幕底卡、卷軸面、引言段落底 |
| `--parchment-dark` 舊紙陰影 | `#c9b083` | 羊皮紙做舊邊緣、摺痕 |
| `--blood-red` 暗紅 | `#7c1f12` | 蒙登/危機/「黑暗紀元」強調字、章節重點 |
| `--subtitle-cream` 字幕米白 | `#f4ecd8` | 主字幕文字(羊皮紙白,不用純白 `#ffffff`,避免廉價數位感) |

### EGA 復古點綴色票(像素元素 / 邊框 / 分隔線)

直接取自標準 CGA/EGA 16 色(這是 U1 DOS 版玩家的視覺記憶,務必忠實):

| EGA 名 | Hex | 建議用途 |
|---|---|---|
| EGA 藍 | `#0000AA` | 像素邊框、太空段背景 |
| EGA 青 cyan | `#00AAAA` | 海洋 tile、月之門光環外圈 |
| EGA 洋紅 magenta | `#AA00AA` | 月之門核心、魔法強調 |
| EGA 棕 brown | `#AA5500` | 地牢、大地 tile |
| EGA 亮黃 | `#FFFF55` | 像素風「按鍵提示」「+1」彈出 |
| EGA 白 | `#FFFFFF` | 僅用於像素 UI / 原版 tile 內,不用於影片文字 |

> 設計原則:**影片文字層用「做舊金 + 羊皮紙白」(避免純白純黃的數位廉價感);像素遊戲截圖層保持 EGA 原色不調色**——讓「古卷的暖」與「像素的冷硬」對比,正是創世紀盒裝(寫實畫)包著(像素遊戲)的本質。

---

## C. 字體

### 中文(主力)

| 用途 | 字體 | 理由 |
|---|---|---|
| 標題卡大標 | **Noto Serif CJK TC Black**(思源宋黑) | 襯線=創世紀招牌的「金字襯線」對應物;Black 字重撐得起史詩感。Linux apt/Google Fonts 皆可得 |
| 副標 / 章節 | **Noto Serif CJK TC SemiBold** | 與大標同家族,層級清楚 |
| 字幕正文 | **Noto Serif CJK TC Medium** | 仍走襯線維持古卷氣;Medium 在 320×200 放大後不糊 |
| (可選)書法重點字 | **TW-Kai 全字庫楷書 / cwTeXKai** | 「黑暗紀元」「薩薩瑞亞」這類專名,用楷書一閃,增手寫卷軸感。整片只點綴,不通篇 |

> ⚠ 不要用黑體(sans)當主視覺——創世紀的 DNA 是**襯線**。黑體會立刻變成現代手遊廣告。

### 英文(標題與裝飾)

| 用途 | 方向 | 具體 |
|---|---|---|
| 英文主標 "ULTIMA" / "The First Age of Darkness" | **古典襯線、寬字距、金色浮雕** | 用 **Trajan / Cinzel**(免費,Google Fonts)這類羅馬碑體大寫,最貼近創世紀盒裝 logo 的金字莊嚴 |
| 符文裝飾(runic) | **Britannian Runes(Elder Futhark 系)** | 角落裝飾、轉場「符文浮現」用。可用 Ultima Codex 社群的 *New Britannia Runic Fonts*(Stroke / Carved / Brushstroke 三款)或 dafont 的 Futhark 系。**只當裝飾紋樣,不要求觀眾讀懂** |

> 創世紀傳統:符文寫的其實仍是英文(路標、墓碑的「替換字體」)。影片用法相同——把中文/英文之外的「氣氛文字」用符文呈現,當作世界觀的視覺暗號。

---

## D. 視覺母題清單 + ffmpeg/ImageMagick 做法

創世紀的「文物宇宙」六大母題,按出現優先序:

| 母題 | 象徵 | 影片用法 |
|---|---|---|
| **羊皮紙 parchment** | 古卷、手冊、史書 | 字幕底卡、引言段背景(最高頻) |
| **安卡 Ankh ☥** | 創世紀的精神符號(美德、生命) | logo 旁、轉場中心、結尾卡簽名 |
| **月之門 Moongate** | 傳送、魔法、「進入另一世界」 | 開場「進入遊戲」轉場、段落切換 |
| **布質世界地圖 cloth map** | 薩薩瑞亞/不列顛全貌 | 世界觀引子段、地圖巡遊 Ken Burns |
| **符文 runes** | 古文明、神秘銘文 | 裝飾角、邊框、章節標 |
| **紋章 / 鎏金邊框** | 王國、莊嚴 | 整片金色描邊框、標題卡外框 |

### 具體製作配方

**1. 羊皮紙底紋(程序生成,不必找素材)**

ImageMagick 用噪聲 + 暖色調做出有機紙感:

```bash
convert -size 1920x1080 xc:'#e8d8b0' \
  \( -size 1920x1080 xc: +noise Gaussian -channel A -blur 0x1 \
     -colorspace Gray -level 40%,75% \) \
  -compose multiply -composite \
  -fill '#c9b083' -colorize 8% \
  parchment.png
# 再疊一層暗角(vignette)做舊:
convert parchment.png \
  \( +clone -fill black -colorize 100% \
     -fill white -draw 'roundrectangle 60,60 1860,1020 40,40' -blur 0x80 \) \
  -compose multiply -composite parchment_aged.png
```

**2. 金色浮雕描邊標題(雙層 stroke + 高光/陰影)**

ffmpeg `drawtext` 疊三層做出鎏金浮雕:暗金陰影(下偏移)→ 主金 → 高光金(上偏移):

```bash
# 概念:同一段文字畫三遍,y 微偏移
drawtext=fontfile=NotoSerifCJKtc-Black.otf:text='黑暗紀元':fontsize=120:\
  fontcolor=0x8a6d1a:x=(w-tw)/2:y=300+3,  # 暗金陰影
drawtext=...:fontcolor=0xd4a017:x=(w-tw)/2:y=300,        # 主金
drawtext=...:fontcolor=0xf0c869:x=(w-tw)/2:y=300-2:alpha=0.5  # 高光
```

或用 ImageMagick 一次做金屬漸層字(更精緻):

```bash
convert -size 1200x200 xc:none -font NotoSerifCJKtc-Black.otf -pointsize 140 \
  -gravity center -fill '#d4a017' -annotate 0 '創世紀' \
  -channel RGBA -blur 0x0.5 \
  \( +clone -background '#8a6d1a' -shadow 80x3+0+4 \) +swap \
  -background none -layers merge +repage title_gold.png
```

**3. 金色裝飾角 / 紋章邊框**

用 ImageMagick 畫四角飾紋(或備一張 PNG 角飾,四角旋轉貼):

```bash
# 整片金框(roundrectangle 描邊)
convert input.png -fill none -stroke '#d4a017' -strokewidth 4 \
  -draw 'roundrectangle 40,40 1880,1040 20,20' \
  -stroke '#8a6d1a' -strokewidth 1 \
  -draw 'roundrectangle 50,50 1870,1030 18,18' framed.png
```

**4. 月之門光環轉場**

紫青(magenta→cyan, EGA `#AA00AA`/`#00AAAA`)的徑向漸層光環,從中心放大吃掉畫面當轉場:

```bash
# 生成月之門光環幀(放大序列),再 overlay
convert -size 1920x1080 radial-gradient:'#00AAAA'-'#0d1b2a' \
  -channel A -evaluate multiply 0.9 +channel moongate_glow.png
# ffmpeg 用 zoompan 讓它從小放大,配 xfade
```

**5. 符文裝飾條(章節分隔)**

用 Britannian runic 字體打一行符文當分隔線,降低不透明度當背景紋樣:

```bash
drawtext=fontfile=BritannianRunes.ttf:text='ULTIMA EXODUS':\
  fontcolor=0xd4a017@0.25:fontsize=40:x=(w-tw)/2:y=h-120
```

---

## E. 影片結構分鏡(60–90 秒)

整體節奏:**慢起(古卷)→ 漸快(遊戲亮點蒙太奇)→ 收於莊嚴(結尾卡)**。建議 70 秒。

### 段 1｜標題卡(0–8s)

- **視覺**:深墨夜藍 `#0d1b2a` 底 + 緩慢浮現的星點 + 中央鎏金浮雕標題「創世紀一代」,下方副標楷書「黑暗紀元」(暗紅 `#7c1f12` 點綴),四角金色飾紋,底部一行半透明符文。
- **動態**:標題從 0 透明度淡入 + 極輕微放大(scale 1.0→1.03),金高光做一次「掃光」(gradient sweep)。
- **字幕語氣**(古卷史詩):無旁白字幕,僅標題本身。
- **音樂**:豎琴/長笛單音前奏起。

### 段 2｜世界觀引子(8–20s)

- **視覺**:切到羊皮紙底卡,布質世界地圖緩慢 Ken Burns 平移(從薩薩瑞亞全圖推近到某城)。
- **字幕**(逐句浮現,襯線白字):
  - 「邪術師蒙登,以不死之寶石奴役薩薩瑞亞。」
  - 「你,是被異界召喚而來的『陌生人』。」
  - 「從最深的地牢,到最遠的星海——」
- **動態**:每句配合地圖緩推;句間用羊皮紙摺痕一閃。

### 段 3｜遊戲亮點投影片(20–58s,核心)

七個亮點各約 4–6 秒,Ken Burns(緩推或緩拉)+ 中文字幕。**截圖保持 EGA 原色**,外加細金框。順序按「由日常到史詩」:

| # | 內容 | 視覺處理 | 字幕(史詩古卷感) |
|---|---|---|---|
| 1 | 世界地圖巡遊(布質地圖風) | 緩推,金框 | 「踏遍薩薩瑞亞的四方疆土」 |
| 2 | 城鎮城堡 | 平移,人物對話框中文 | 「於王城求見不列顛君王」 |
| 3 | 第一人稱地牢 | 緩拉(由暗轉明),暗角加重 | 「深入不見天日的地底迷宮」 |
| 4 | 戰鬥畫面 | 快推,血紅閃一下 | 「以劍與魔法迎戰蒙登的爪牙」 |
| 5 | 星海太空戰(U1 簽名!) | 切換感最強,EGA 藍黑底 | 「駕太空梭,於繁星之海纏鬥」 |
| 6 | 七平台美術對照(Apple/C64/NES/CGA/EGA/VGA…) | 快速拼貼/格狀並陳 | 「橫跨四十年的經典,如今重生」 |
| 7 | 時光機 / 勝利畫面 | 月之門光環包裹 | 「啟動時光機,回到黑暗未臨之時」 |

> 第 5、7 是 U1 的靈魂(科幻反差 + 時光機殺蒙登),務必給足停留。第 6 順帶展示 remake 的多平台 tileset 賣點。

### 段 4｜結尾卡(58–70s)

- **視覺**:漸暗回深墨夜藍,中央大安卡 ☥ 金色浮現(發光),其下:
  - 主標「創世紀一代:黑暗紀元」
  - 副標「繁體中文版 · SDL2 復刻」
  - 底部:GitHub repo / 「免費開源」字樣(米白小字)
- **動態**:安卡從暗到亮緩緩發光,星點繼續閃;最後一切定格 2 秒後淡出。
- **音樂**:主題收束,留一記長音/餘韻。

---

## F. 轉場與節奏

| 段落間 | 轉場 | ffmpeg 手法 |
|---|---|---|
| 標題→引子 | 羊皮紙「翻頁」/ 淡入 | `xfade=transition=fade` 或自製紙張滑入 overlay |
| 引子→亮點 | **月之門光環吞噬**(招牌) | 紫青徑向漸層放大 + `xfade` |
| 亮點之間 | 快節奏交叉溶解 / 金光一閃 | `xfade=fade` 0.4s,配合節拍 |
| 亮點→結尾 | 漸暗(vignette 收緊) | overlay 暗角 + `fade=out` |

節奏原則:
- **前 20 秒每幀停留長(2–4s)**,營造古卷莊嚴;**亮點段縮到 1.5–2.5s 並卡音樂節拍**;結尾再放慢。
- Ken Burns 幅度小(scale 1.0→1.08,平移 < 10%),**緩慢勻速**——創世紀是莊嚴不是花俏,忌快速甩鏡。
- 轉場時長 0.3–0.6s,寧短勿炫。

---

## G. 配樂方向

- **氣質**:中世紀輓歌/吟遊(medieval lay)——長笛、豎琴、弦樂,莊嚴、略帶哀愁、史詩。創世紀招牌曲《Stones》(David Watson 作,靈感來自奧克尼群島的石圈)正是這個調性:flute + harp + strings。
- **影片用樂**:
  1. **首選**:用本 remake 的**遊戲實機音樂**(忠於專案、無版權疑慮)。若原版 U1 為 PC speaker/chiptune,可考慮用 remake 重編的管弦/midi 版。
  2. 結構配合分鏡:前奏(豎琴單音)對標題;主題展開對亮點段;收束長音對結尾安卡。
  3. **避免**:史詩 trailer 式的重鼓 BWAAAM、電子音、好萊塢 epic——那是通用模板,毀掉創世紀的古樸吟遊感。
- **音量**:音樂為主(無旁白時 −6dB 滿;若加旁白則 ducking 到 −18dB)。亮點段可在「太空戰」「時光機」兩處讓音樂揚起一個樂句。

---

## H. 與既有 pipeline(Indiana Jones 專案)的對接

既有 Indiana 做法:**深藍底 `#0a0e1a` + 金字 `#f0c000` + Noto CJK Black + Ken Burns + 遊戲音樂 + ffmpeg 投影片合成**。骨架可直接沿用,以下五點改造成「創世紀味」:

| 維度 | Indiana 既有 | 創世紀版改法 |
|---|---|---|
| **背景底色** | 純深藍黑 `#0a0e1a` | 改 `#0d1b2a`(帶藍綠的深夜星空),並**加細微星點 + vignette**——創世紀是「夜空下的古卷」,不是冒險動作片的黑 |
| **標題金** | 明亮黃金 `#f0c000`(sans 感) | 改**古銅鎏金 `#d4a017` + 三層浮雕**(暗金 `#8a6d1a` 陰影 + 高光 `#f0c869`)——要「鎏金浮雕」不要「螢光黃」 |
| **字體** | Noto **Sans/黑體** CJK Black | 換 Noto **Serif** CJK TC Black(襯線是創世紀 DNA);英文標題改 Trajan/Cinzel 碑體;加 Britannian 符文當裝飾 |
| **底卡材質** | 純色深底 | 部分段落改**羊皮紙底卡**(程序生成,見 D-1),字幕坐在羊皮紙上而非純色 |
| **母題/轉場** | 一般 fade/Ken Burns | 加入**安卡 ☥ / 月之門光環轉場 / 金色裝飾角 / 符文分隔**——這四個是「立刻認出是創世紀」的視覺鉤子 |
| **截圖調色** | (視情況) | **像素截圖保持 EGA 原色不調色**,僅加金框;讓「暖古卷框 × 冷像素內容」對比成立 |

> 一句話:**沿用 Indiana 的 ffmpeg 合成腳本與 Ken Burns 機制,只換「色票 + 襯線字體 + 羊皮紙底 + 安卡/月之門母題」這四個皮**,即可從「通用復古遊戲 trailer」轉成「創世紀古卷史詩」。改動集中在 drawtext 參數、底圖素材、轉場 overlay 三處,工程量 bounded。

---

## 來源 URL 清單

**封面美術 / Denis Loubet**
- https://wiki.ultimacodex.com/wiki/Denis_Loubet
- https://en.wikipedia.org/wiki/Denis_Loubet

**Ultima I 視覺 / 劇情(太空梭、時光機、蒙登)**
- https://www.wiki.ultimacodex.com/wiki/Ultima_I
- https://en.wikipedia.org/wiki/Ultima_I:_The_First_Age_of_Darkness
- https://www.hardcoregaming101.net/ultima-i-first-age-of-darkness/

**符文 / 字體**
- https://wiki.ultimacodex.com/wiki/Runic_alphabet
- https://lycaeum.ultimacodex.com/new-britannia-runic-fonts-6-styles/

**母題:安卡 / 布質地圖 / 羊皮紙手冊**
- https://wiki.ultimacodex.com/wiki/Ankh
- https://wiki.ultimacodex.com/wiki/Ultima_IV_map_of_Britannia

**配色:EGA/CGA 色票**
- https://moddingwiki.shikadi.net/wiki/EGA_Palette
- https://int10h.org/blog/2022/06/ibm-5153-color-true-cga-palette/

**音樂:Stones / Ultima 配樂**
- https://wiki.ultimacodex.com/wiki/Stones
- https://wiki.ultimacodex.com/wiki/Music
