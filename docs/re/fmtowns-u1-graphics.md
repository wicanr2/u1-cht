# FM Towns Ultima I 圖檔對應(E1 進行中)

> 來源:Trilogy CD(`u7-cht/…FM_TOWNS…`),GRAPH 目錄**已抽出**於
> `u7-cht/fmtowns_work/graphout/GRAPH/`(u2-cht 既有成果,免重抽 CD)。
> 解碼:`tools/re/fmtowns_decode.py`(TIF、FillOrder=2 LSB、4bpp;見 u2-cht 經驗)。

## U1 相關 TIF(UT1*)

| 檔 | 大小 | 內容(已解碼目視) |
|---|---|---|
| `UT1MAP.TIF` | 33280 | **地形**(水/森林/城門/磚牆…)+ **字型 A-Z**(底部) |
| `UT1TILE0.TIF` | 33280 | **怪物 / 角色 sprite**(32×32,多色立繪) |
| `UT1TILE1.TIF` | 33280 | **物件 / 載具 / 建築**(城堡/城鎮/馬/車/筏/船/太空梭/時光機…) |
| `UT1_1..4.TIF` | 87378 | 全螢幕場景圖 |
| `UT1SPACE.TIF` | 33280 | 太空序列 |
| `UT1T_1..5.TIF` | 15-35K | 標題畫面 |

- sprite 原生 **32×32**(engine tile 是 16×16 → 需 downscale)。
- 排列:32px 寬連續長條(strip),每 32 列一個 sprite。

## ⚠️ 待校準(完成 E1 FM Towns 包前必處理)
1. **palette 不對**:現用 u2-cht 的 U2 palette → U1 森林顯示**紅色**(應綠)。需找 U1 FM Towns 專屬
   palette(對 `reference/hg101/` 的 FMTOWNS 實機截圖校準)。
2. **52 格對應**:engine 需 Water,Grass,Forest,Mountain,Castle×2,…(共 52,見 TileTypeLoader)。
   FM Towns 分散在 MAP(地形)+TILE1(物件)+TILE0(怪物)→ 逐格對應(provisional,視覺判定)。
3. **32→16 downscale**:組成 832×16 PNG(對齊 E0 的 PNG AssetPack 格式)。

## 流程(E1 續)
解 3 個 TIF → 校 palette → 逐格挑 sprite 對 engine 52 槽 → downscale → 拼 832×16 PNG →
`config tileset_png=assets/tilesets/fmtowns.png` 載入 → 對實機截圖校驗 → commit。

## palette 校準進展(2026-06-26)

- TIF 開頭 `00 00 00 00`(非標準 TIFF header)→ **無內嵌 ColorMap**,palette 須外求。
- u2 的 `fmtowns_pal.json` / `derived_palette.json` 是 **U2 專屬**(每遊戲執行期自設 palette),套 U1 不準。
- **進展**:改用**完整 16 色 RGBI palette + bit 反序(FillOrder=2)**重解 `UT1MAP` →
  地形結構認得出(樹/城門/磚牆/水/**草地變綠**),但仍有紅/洋紅雜點 → 部分 index 對應未中。
  工具 `tools/re/fmtowns_dec16.py`。
- **剩餘**:取得 **U1 專屬 16 色 palette** ——
  路徑 A:從 U1 FM Towns 遊戲檔(執行檔/資源)RE 出 palette 暫存器值;
  路徑 B:對 `reference/hg101/imgs/ultima1-FMTOWNS-06.png`(U1 overworld 實機)逐色採樣校準。
- ground truth 顏色(取自實機 06):草=亮綠、水=藍波紋、森林=深綠叢、山=深色藍邊、玩家=藍衣。

## ★ 解碼破解(2026-06-26):格式 = chunky 4bpp + FillOrder=2 反序 + 亮 RGB-16

- 從 hg101 FMTOWNS-06 實機萃取遊戲區用色:草=`#00FF00`、水=`#0000FF`/`#00FFFF`、黃=`#FFFF00`、白/黑
  → **U1 用純亮 RGB**(非 EGA 暗色 A8/54)。
- 正確解碼:**chunky 4bpp(每 byte 2 像素)+ 每 byte bit 反序(FillOrder=2)+ 亮 RGB-16 palette**。
  (chunky 不反序 → 草地解成紅 index4;反序把 index 2↔4,草地正確變綠。)
- 工具:`tools/re/fmtowns_u1_decode.py`(輸出 32×32 tile 網格)。成果:`docs/re/img/ut1map_grid.png`。
- **UT1MAP tile 佈局**(8×8=64 格,32×32):前 ~20 格地形(草/森林/水/城門/磚牆),中段稀疏,
  末 2 列(~48-63)= **字型 A-Z**。
- 殘留:草地紋理個別 index 仍偏紅(亮 palette 的 index 4/12 在草紋處應為深綠)→ 微調 palette texture index。

## E1 剩餘(解碼已通,剩組裝)
1. palette texture index 微調(草紋紅→深綠)。
2. 52 槽對應:UT1MAP(地形)+ UT1TILE1(物件:城堡/城鎮/馬/車/船/梭/時光機)+ UT1TILE0(怪物)。
3. 32→16 downscale → 拼 832×16 PNG → `tileset_png` 載入 → 對實機校驗。

## ★ 里程碑(2026-06-26):FM Towns 素材包載入遊戲成功(provisional)

- `tools/build_fmtowns_pack.py` 組出 832×16 FM Towns 包(地形自動分類 + 物件/怪物 + 32→16 downscale)。
- `config tileset="png" / tileset_png="assets/tilesets/fmtowns.png"` → **FM Towns 亮綠草地渲染在 open_ultima**
  (見 `docs/re/img/fmtowns_ingame.png`)。**跨平台 PNG AssetPack pipeline 端到端打通。**
- **provisional 待修**:
  - **water 顯紅**:UT1MAP 前 24 格用我的近似 palette 解不出純藍(`water blue%=0`)→ 水的 index 對到非藍。
  - grass 紅雜點:texture index 對應。
  - 根因:**缺 U1 精確 palette**。「依 nibble bit 推亮色」讓 grass=綠對,但 water 等不對。
- **續作(精確 palette 推導)**:
  1. 取已知 tile(grass 全綠/water 全藍,對 hg101 實機)→ 找其主要 nibble index →
     建 index→實機色 對照,逐 index 校準完整 16 色 palette。
  2. 52 槽精確對應(water/grass/forest/mountain/物件/怪物各挑對 tile)。
