# Ultima I: First Age of Darkness — 繁體中文化

基於 [`matiaslaino/open_ultima`](https://github.com/matiaslaino/open_ultima)(C++17 / SDL2,MIT)的
繁體中文化與 Linux 移植專案。

> 狀態:**Phase 0 鳥巢框架完成**,Phase 1(Linux 可編譯)進行中。詳見 [`PLAN.md`](PLAN.md)、[`CONTEXT.md`](CONTEXT.md)。

## 這是什麼
把 open_ultima(Ultima I 開源重製,目前開發極早期:世界地圖 + 基礎地牢)在 Linux 上 build,
並換上 CJK 字庫 + UTF-8 字串管線做繁中化。

## 建置(Docker)
```bash
docker build -t u1-cht docker/
docker run --rm -v "$PWD":/work u1-cht \
  bash -c "cmake -S . -B build && cmake --build build -j"
```

## 執行需求
- SDL2 / SDL2_image / SDL2_ttf / SDL2_gfx、nlohmann/json(Docker 內已備)。
- **原始遊戲資料檔**(版權,使用者自備,來自 GOG 版 U1):放入 `gamedata/`。
- `config.json`(複製 `config.json.example` 調整路徑)。

## 操作
見 [`docs/keybindings.md`](docs/keybindings.md)。ESC=返回、F10=離開(確認+自動存檔)。

## 授權
本專案程式碼承上游 MIT(見 `LICENSE.open_ultima`)。原始遊戲資料為版權所有,不隨附。
