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
