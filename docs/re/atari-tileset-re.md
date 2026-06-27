# Atari 8-bit《Ultima I》overworld tileset 逆向(進行中)

> 目標:取出 Atari 8-bit 版(1983)的完整 overworld tileset(對齊 engine 52 槽)。
> 方法依 L.CY 指示:**用已抽的檔 + 遊戲實際圖比對,不跑模擬器**(`rulebook/64` 截圖 oracle、
> `rulebook/83` 完整性 > 投報)。素材:`re_work/atari/`(ATR 已抽各 `*.bin` segment,版權,gitignore)。

## 已確立

1. **檔案格式**:`OUTMOVE.bin`(overworld 程式)等為 **Atari binary load 格式**
   (`FFFF` magic + 多個 `[start LE][end LE][data]` segment)。`tools/`(本目錄)解析器重建記憶體影像。
2. **文字字型 charset @ $6400**:反組譯 OUTMOVE 找到 `CHBAS($02F4)=$64`、`CHBASE($D409)=$64`
   → charset 在 **$6400**。但 segment 只載入 chars 0-18($6400-$6497)+ chars 32-50($6500-$6597)
   = 38 個重定義字模,**其餘為全零**。
3. **★ 關鍵否證:overworld tile 圖不在 $6400 charset**。
   用實機圖反推驗證:把 atari800-04 的 water/grass/player tile 轉成 ANTIC 4/5 邏輯像素
   (每字元 **4 px 寬 2bpp、ANTIC5 每列顯示 2 次**),結構比對 $6400 全 128 字元 → **零命中**
   (grass 只命中全零空字元)。⇒ $6400 是**文字字型**,地圖 tile 由別的機制繪製。

## 已取得(authentic)

**4 個真實 Atari tile**(`rip_atari_tiles.py` 從 atari800-04 切,16×16):
- **water**:藍色波紋 / **grass**:橄欖綠短橫點 / **forest**:橄欖綠密塊 / **player**:灰色人形。
- overworld palette(由截圖反推):**黑底 + 橄欖綠(75,88,0)/ 藍(8,64,212)/ 灰(160,160,160)**。
  注意:OUTMOVE @$8103 的 COLPF0-3=$84/$d4/$56/$0a 是**別場景**的色,非 overworld。

## 待解(完整 52 槽的下一步)

tile 圖既不在 $6400 charset,候選儲存/繪製機制(逐一驗):
1. **第二套 charset**(經 DLI 在地圖區切 CHBAS;或 `SET1-5.bin` 載到 $2400 的另一字模集)——
   `SET1-5.bin` 連續載 $2400-$2a59,先前當 charset 渲為雜訊,需配正確 ANTIC 模式/色再試。
2. **軟體 bitmap / PMG**:tile 可能畫進點陣畫面緩衝,非字元。
3. **map-draw routine RE**:反組譯 OUTMOVE 的逐格繪製(讀 map cell → 取 tile 圖 → 寫畫面),
   反追 tile 圖指標到實際儲存位置(同 IIgs 的 `$4c00`/DrawTile 追法)。需 6502 反組譯器。
4. **更多實機截圖**:單張 overworld 只 4 tile 入鏡;town/castle/dungeon/載具/怪物需各自截圖
   或同源 Apple II 版(tile 近似)補。

> 進度誠實標示:格式與文字字型已定位、4 tile authentic;完整 tileset 待上述其一突破。
> 不因「4 色粗圖」而放棄——`rulebook/83`:老遊戲整合判準是完整性,保全歷史。
