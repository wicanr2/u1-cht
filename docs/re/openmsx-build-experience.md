# 在 Docker 裡編 openMSX + 跑磁碟遊戲 dump VRAM — 建置經驗

> 緣由:E3 MSX tileset 靜態解不出 → 需 openMSX 跑到 overworld、dump VRAM 當 ground truth。
> openMSX 不在 Ubuntu 24.04 apt → 源碼編。本檔記錄完整流程 + 踩雷,可複用於任何「源碼編模擬器 in docker」。
> 配 `docs/re/openmsx-setup.md`(BIOS/機器 config)。

## 0. 核心策略:build 在 docker,binary 留主機(mount 持久化)
- 用 `docker run --rm -v "$PWD":/work` 在容器內編譯,**源碼 clone 到 mounted 的 `re_work/`**。
- ⇒ 容器 `--rm` 砍掉後,**編出的 binary 仍在主機 `re_work/openMSX/derived/.../bin/openmsx`**。
- **不污染系統、可重現、不必把整套 build 鏈烤進 image**(符合 docker-first 硬規則)。
- 替代:烤進 `Dockerfile.re` —— 但那會在每次 image build 重編(慢),且無法重用 mounted binary。
  **本案選 build-to-mount**,因為只編一次、binary 持久。

## 1. ★ build deps vs runtime libs(關鍵踩雷)
- **build 容器是 `--rm`**:`apt-get install` 的套件**容器結束就消失**。
- 編譯需 `-dev` 套件;**執行**只需 runtime 共享庫(`.so`)。兩者不同包:
  | 階段 | 套件 |
  |---|---|
  | build | `g++ make python3 pkg-config libsdl2-dev libsdl2-ttf-dev libpng-dev tcl-dev libogg-dev libtheora-dev libvorbis-dev libglew-dev` |
  | run | `libsdl2-2.0-0 libsdl2-ttf-2.0-0 libtcl8.6 libpng16 libtheora0 libvorbisfile3 libglew2.2 + xvfb` |
- ⇒ **跑 openmsx 的新容器要再裝一次 runtime libs**(或同一容器 build+run 不退出)。

## 2. 編譯步驟(openMSX)
```bash
git clone --depth 1 https://github.com/openMSX/openMSX.git
cd openMSX && ./configure && make -j$(nproc)
# configure 印 "All required and optional components can be built." = deps OK
# 產物:derived/<arch>-linux-opt/bin/openmsx
```
- openMSX 用自家 Python build 系統(非 autotools);`make` 是大型 C++ 專案,**數分鐘~半小時**。
- `| tail -N` 會緩衝:make 中途看不到輸出,完成才吐尾。要看進度別接 tail。

## 3. ★ BIOS:用 sha1 對上內建機器(省自訂)
- openMSX **按 sha1 掃 `systemroms/` 認 ROM**(檔名無關)。
- 我們從 `github.com/archtaurus/RetroPieBIOS/BIOS` 抓的**通用** `MSX2.ROM`/`MSX2EXT.ROM` 的 sha1
  **正好等於內建 `Philips_NMS_8245` 機器的 `nms8245_basic-bios2.rom`/`nms8245_msx2sub.rom`**:
  - `MSX2.ROM` sha1 `6103b39f…` ✓ ・ `MSX2EXT.ROM` sha1 `5c1f9c7f…` ✓
- ⇒ 放進 systemroms 即被 NMS8245 認得。唯 `DISK.ROM`(`032cb1c1…`)是通用磁碟 ROM、
  與 8245 內建 disk ROM sha1 不同 → **自訂機器 config 把 disk ROM 加上我們的 sha1**(`tools/re/msx/U1MSX2.xml`)。
- ⚠ 通用 DISK.ROM 須與機器 FDC 型號相容(WD2793 vs TC8566 vs MB8877);不相容會開機卡磁碟。

## 4. Headless 跑 + dump VRAM
```bash
xvfb-run -a openmsx \
  -machine U1MSX2 -diska "U1.dsk" -script dump.tcl
# 環境:OPENMSX_USER_DATA 指向含 machines/U1MSX2.xml + systemroms/*.ROM 的目錄
```
`dump.tcl`(`tools/re/msx/dump.tcl`):`after time N { debug save_to_file "physical VRAM" 0 0x20000 vram.bin; set_screenshot … }`。
- ⚠ tileset 要進 VRAM 需遊戲跑到 overworld(過 title+建角)→ Tcl `keymatrixdown`/`type` 自動敲鍵。
  先 dump title 驗證開機/磁碟 OK,再推進。

## 5. 踩雷清單
- [雷] `--rm` build 容器 → runtime libs 消失;run 要重裝(見 §1)。
- [雷] `make | tail` 緩衝 → 中途無進度;別接 tail 才看得到。
- [雷] C-BIOS 無磁碟機 → 跑 .dsk 必用真實 BIOS + DISK.ROM(見 `openmsx-setup.md`)。
- [雷] 通用 DISK.ROM 的 FDC 相容性 → 不對會卡開機。
- [雷] 日版遊戲可能需 KANJI.ROM 才不卡開機(文字)。

## 6. 結果(2026-06-26,已驗證)
- [x] **編譯成功**:`make -j` 數分鐘,binary = `re_work/openMSX/derived/x86_64-linux-opt/bin/openmsx`。
- [x] **runtime 執行 image**:`docker/Dockerfile.msxrun`(FROM u1-cht + runtime libs)避免反覆裝。
- [x] **BIOS 開機**:U1MSX2 機器(MSX2.ROM+MSX2EXT.ROM 對上 NMS8245 sha1,DISK.ROM window 修 0x0000)
  → MSX BIOS 開機(VIDEO/USER RAM 128K)→ **MSX-DOS 1.03 從磁碟開機成功**。
- [x] **跑遊戲**:MSX-DOS prompt `type "OUT\r"` → **Ultima I 片頭播放**(Ponycanyon 日版 title/ORIGIN logo)。
- [x] **抓到 VDP 模式 + palette**(`debug read_block {VDP regs}/{VDP palette}`):
  - 模式 = **SCREEN 7**(R0=0A M5=1 M3=1)。
  - palette = **8 色 GRB**:`0黑 1綠 2紅 3黃 4藍 5青 6洋紅 7白`(index = B<<2|R<<1|G)。**先前 MSXTILES 解碼 palette 全錯,這是正解。**
- [ ] **MSXTILES 解碼**:仍需到 overworld(OUT 載 tile 進 VRAM)dump → 比對。卡在過片頭+建角的鍵序導航。
  - 已試靜態各佈局 + 正確 palette 仍雜訊 → 確定要 overworld VRAM 當 ground truth。

## 7. dump 關鍵 API(這版 openMSX)
- `debug list` 列可讀區塊:`{physical VRAM}` `{VDP palette}` `{VDP regs}` `{CPU regs}` `keymatrix` …
- **dump**:`set d [debug read_block "physical VRAM" 0 [debug size "..."]]` + Tcl `open ... wb` 寫檔
  (注意:**舊的 `debug save_to_file` 已移除**,用 `read_block`)。
- **截圖**:`screenshot -raw out.png`。
- **敲鍵**:`type "OUT\r"`(ASCII + \r);特殊鍵 `keymatrixdown/up`。
- **定時**:`after time <秒> { ... }`(emulated time)。

## 8. 遊戲流程(導航,2026-06-26)
- `OUT` → 片頭 credits 序列(Ultima I / ORIGIN / Lord British,很長,連敲 space 推進)→ **主選單**:
  - `*** Ultima I ***`、`a) キャラクターさくせい`(建角)、`b) ゲームスタート`(開始)、`どちらにしますか?`
- ⚠ space/return 不選;要敲 **`a`**(建角,多步驟日文:名/能力/種族/職業)→ 回選單敲 **`b`** 開始 → overworld。
- 敲 `b` 未建角無效(停選單)。**MSXTILES 在 overworld 才載入 VRAM** → 需先建角。
- throttle:`set throttle off` 讓模擬全速(否則 xvfb 下模擬時間 << 真實,長序列超時)。
- 截圖證據:`docs/re/img/msx_dos.png`(MSX-DOS 開機)、`msx_menu.png`(主選單)。

## 9. 續作(MSXTILES 解碼,剩最後一哩)
1. 腳本敲 `a` → 走完建角(名+能力配點+種族+職業)→ 回選單敲 `b` → overworld。
2. dump VRAM(此時 MSXTILES 已載入)→ 自動搜尋 MSXTILES.BIN 內容定位 → 讀出 SCREEN 7 佈局。
3. 用已知 palette(8色GRB)+ 實際佈局寫解碼器 → 832×16 PNG(同 E1 流程)。
