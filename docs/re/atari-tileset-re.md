# Atari 8-bit《Ultima I》overworld tileset 逆向(✅ 完成)

> **TL;DR**:tile 在 `$6400` charset,但是 **1bpp(每 byte 8 像素)非 2bpp**(這是先前卡關主因)。
> 一個 overworld tile = **char[i](上半 8×8)+ char[i+32](下半 8×8)= 8×16**,共 **19 個 tile**。
> 兩段 charset($6400 chars 0-18、$6500 chars 32-50)正好是 19 個 tile 的上半與下半。
> palette(實機圖反推):水=藍、植被=橄欖綠、結構/玩家/生物=灰,黑底。
> `build_atari_pack.py` → `assets/tilesets/atari.png`,game tester 渲染與 atari800-04 實機圖一致。


> 目標:取出 Atari 8-bit 版(1983)的完整 overworld tileset(對齊 engine 52 槽)。
> 方法依 L.CY 指示:**用已抽的檔 + 遊戲實際圖比對,不跑模擬器**(`rulebook/64` 截圖 oracle、
> `rulebook/83` 完整性 > 投報)。素材:`re_work/atari/`(ATR 已抽各 `*.bin` segment,版權,gitignore)。

## 已確立

1. **檔案格式**:`OUTMOVE.bin`(overworld 程式)等為 **Atari binary load 格式**
   (`FFFF` magic + 多個 `[start LE][end LE][data]` segment)。`tools/`(本目錄)解析器重建記憶體影像。
2. **文字字型 charset @ $6400**:反組譯 OUTMOVE 找到 `CHBAS($02F4)=$64`、`CHBASE($D409)=$64`
   → charset 在 **$6400**。但 segment 只載入 chars 0-18($6400-$6497)+ chars 32-50($6500-$6597)
   = 38 個重定義字模,**其餘為全零**。
3. **走錯的彎路(記錄以備鑑)**:先以為 tile 是 **2bpp ANTIC 4/5**(4px/char),結構比對 $6400 全零命中,
   一度誤判「tile 不在 $6400」。錯在 **bpp**:Atari overworld 的 tile charset 是 **1bpp**(ANTIC 2 類,
   每 byte 8 像素),不是多彩 2bpp。換 1bpp 解讀後字模立刻清晰可辨。

## ★ 破解:1bpp charset + 上下半合成

用「已知輸出反推」(`rulebook/64` 截圖 oracle):把 atari800-04 的 **player tile**(最複雜、不會誤中)
轉 8×16 1bpp,搜遍記憶體 → 上半 8 byte 命中 **$6440 = char 8**。⇒ tile 是 **1bpp**、存在 $6400。
再對 4 個實機 tile 求上下半 char:

| tile | 上半 char | 下半 char |
|---|---|---|
| grass | 1 | 33 |
| forest | 2 | 34 |
| player | 8 | 40 |

**規律:下半 = 上半 + 32**。兩段 charset(chars 0-18 + 32-50)就是 19 個 tile 的上半與下半。
⇒ **tile i = char[i](上)+ char[i+32](下),8×16,共 19 個**。1bpp 全部渲出 → 水/草/林/山/城堡/
城鎮/地牢/玩家/馬/載具/怪物全部可辨。

palette(實機圖反推,overworld 僅 3 前景色):**水=藍(8,64,212)、植被(草/林/山)=橄欖綠(75,88,0)、
其餘(結構/玩家/生物)=灰(160,160,160)**,黑底。

## 成果

- `build_atari_pack.py`:解 19 個 1bpp tile(char i + i+32)、套實證 palette、19→52 槽 → `assets/tilesets/atari.png`。
- game tester 進 overworld 渲染與 atari800-04 實機圖**一致**(藍波紋水、橄欖綠草點、綠森林、灰人形玩家)。
- 工具:`rip_atari_tiles.py`(截圖切 oracle tile)、`build_atari_pack.py`(主建構)。

> 教訓:卡關時別只懷疑「位置」,也要懷疑「**格式假設**」(這裡是 bpp)。同一份 $6400 資料,
> 2bpp 解讀是雜訊、1bpp 解讀是乾淨 tile。`rulebook/83`:不因「4 色粗圖」放棄——完整性,保全歷史。
