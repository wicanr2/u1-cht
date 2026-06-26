# Ultima I: First Age of Darkness — 繁體中文化

基於 [`matiaslaino/open_ultima`](https://github.com/matiaslaino/open_ultima)(C++17 / SDL2,MIT)的
繁體中文化、Linux 移植與功能補完專案。

> 狀態:可在 Linux 執行,世界地圖全中文、宋體抗鋸齒、回合制生怪與地面戰鬥。
> 完整規劃見 [`PLAN.md`](PLAN.md);專案語彙與背景見 [`CONTEXT.md`](CONTEXT.md)。

## 這是什麼
把 open_ultima(Ultima I 開源重製,上游開發極早期:世界地圖 + 基礎地牢)在 Linux 上 build,
換上 CJK 字庫 + UTF-8 字串管線做繁中化,並逐步補完遊戲功能(對齊原版 U1 行為)。

## 已完成
- **全畫面繁體中文**(世界地圖訊息、狀態列、戰鬥、敵人名);UTF-8 管線。
- **拉高內部畫布 320→640**:底圖 nearest crisp 放大,中文用 16px 不糊。
- **宋體(Noto Serif CJK)抗鋸齒**,1280×800 乾淨 2× 縮放。
- **F10/Ctrl+Q 離開確認 + ESC 取消**(置中中文對話框)。
- **tileset 熱鍵(F1/PageDown)**EGA/CGA 切換 + 螢幕提示(切換不再讓 NPC 消失)。
- **音樂系統(SDL_mixer)+ M 鍵切換**(占位音樂,FM Towns 真音樂後續)。
- **回合制生怪 + 地面怪相鄰反擊**(對齊原版 U1:地面怪不移動)。

## 文件索引
| 文件 | 內容 |
|---|---|
| [`PLAN.md`](PLAN.md) | 執行計畫、階段拆解、技術堆疊 |
| [`CONTEXT.md`](CONTEXT.md) | 專案語彙(glossary)、上游現況、硬規則 |
| [`docs/keybindings.md`](docs/keybindings.md) | 操作介面與快速鍵(含進度) |
| [`docs/game-mechanics.md`](docs/game-mechanics.md) | 時間 tick、怪物生成率、切 tileset 的 NPC 處理(參考 u2-cht) |
| [`docs/ultima1-original-ai.md`](docs/ultima1-original-ai.md) | 原版 U1 (1981) 怪物 AI 考據(地面不動、地牢會追)+ 出處 |
| [`docs/localization-notes.md`](docs/localization-notes.md) | 中文化技術筆記(字型管線、拉畫布決策) |
| [`docs/materials.md`](docs/materials.md) | 原始遊戲 / 各版本素材來源清單(DOS/MSX/PC98/FM Towns…) |
| [`docs/adr/0001-pluggable-asset-packs.md`](docs/adr/0001-pluggable-asset-packs.md) | 可替換 tileset/sprite 素材包架構決策 |

## 建置(Docker)
```bash
docker build -t u1-cht docker/
docker run --rm -v "$PWD":/work -w /work u1-cht \
  bash -c "cmake -S . -B build && cmake --build build -j"
```

## 執行需求
- SDL2 / SDL2_image / SDL2_ttf / SDL2_gfx / SDL2_mixer、nlohmann/json(Docker 內已備)。
- **原始遊戲資料檔**(版權,使用者自備,來自 GOG 版 U1):放入 `gamedata/`。
- `config.json`(複製 `config.json.example` 調整路徑;可調 `speed_pct` / `spawn_pct`)。

## 測試(game tester)
```bash
docker run --rm -v "$PWD":/work -w /work u1-cht bash tools/game_tester.sh
# 產出 tests/snapshots/out/*.png,驅動正常玩家路徑(移動/tileset/音樂/離開)逐步截圖
```

## 授權
本專案程式碼承上游 MIT(見 `LICENSE.open_ultima`)。原始遊戲資料、各平台美術/音樂為版權所有,不隨附。
