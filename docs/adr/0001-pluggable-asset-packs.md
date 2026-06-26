# ADR 0001 — 可替換的 tileset / sprite「素材包」架構

狀態:提議(2026-06-26)・ 對應需求:使用者要「能替換 tileset & sprite 跟不同版本」。

## 背景
- open_ultima 以 **DOS 版**為基礎,內建兩種像素解碼:`EGARowPlanarDecodeStrategy`、`CGALinearDecodeStrategy`,
  執行期可在 EGA/CGA 間切換(`usingEga` toggle)。原始資料另含 **Tandy 1000**(`T1K*TILES.BIN`)同類格式。
- 使用者希望能換上**其他版本的美術**:Apple II / IIgs / Atari 8-bit / C64 / PC-88 / PC-98 / MSX2 / FM Towns。
  這些平台的圖檔格式各異,與 DOS BIN 完全不同。
- 參考截圖已備:`reference/hg101/imgs/`(hardcoregaming101,9 平台 42 張,供拆 sprite 比對)。

## 決策
引入 **AssetPack(素材包)抽象**,讓「tileset/sprite 來源」與遊戲邏輯解耦。一個素材包提供
overworld tiles / town tiles / sprites,來源可為:

1. **原始 BIN 解碼**(現有路徑):DOS EGA / CGA / Tandy → 既有 DecodeStrategy。
2. **PNG sprite sheet**(新路徑):任一平台的美術(自截圖/原始資產轉出的點陣圖)
   經統一切版規格,用 `SDL_image` 載入。中文化字型亦走此層(CJK TTF)。

選用哪個素材包由 `config.json` 的 `tileset` 指定(如 `ega` / `cga` / `tandy` / `fmtowns` / `appleii`)。

## 第一步(可立即做,低風險)
把現有「EGA/CGA 硬編碼 + 執行期 toggle」改成 **config 驅動的 tileset 變體選擇**
(`ega` / `cga` / `tandy` 三者皆為現成 BIN,格式相容),作為 AssetPack 抽象的雛形。
其餘平台(PNG 來源)待 sprite 拆解工作展開後再接。

## 影響
- `Configuration` 增 `tileset` key 與對應檔名解析(已預留 `getGameFilesPath`)。
- `main.cpp` tileset 載入改讀 config,不再寫死 `usingEga=true`。
- 新增 `AssetPack` 介面 + 兩種實作(BIN-decode / PNG-sheet)為後續工作。
- 非 DOS 平台美術為版權素材,**不入庫**;轉檔流程與來源另記。

## 未決
- 各平台原始圖檔的取得與解碼(Apple woz / FM Towns 等)工程量,待逐版評估。
- 統一 PNG sprite sheet 的切版規格(tile 尺寸、索引、遮罩)需定義。
