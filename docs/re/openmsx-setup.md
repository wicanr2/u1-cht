# openMSX 環境建立流程(跑 MSX U1 .dsk → dump VRAM 解 tileset)

> 目的:E3 MSX tileset 格式靜態解不出 → 用 openMSX 跑到 overworld,dump VRAM 當 ground truth
> 比對 MSXTILES.BIN 佈局 + 校 palette。
> 第一性原理:模擬器要忠實跑 = 還原原機的 **CPU + VDP + 系統 ROM + 磁碟**。逐項備齊。

## 1. 為什麼 C-BIOS 不夠 → 需真實 BIOS
- openMSX 內建 **C-BIOS**(開源 BIOS)**只支援 cartridge,無磁碟機**(已驗 `C-BIOS_MSX2+.xml` 無 FDC)。
- 跑 `.dsk` 必須:真實 MSX2 機器 + **DISK.ROM**(磁碟 BIOS)。日版遊戲另需 **KANJI.ROM**。

## 2. 需要的 BIOS(從第一性原理:MSX2 開機鏈)
| ROM | 大小 | 角色 |
|---|---|---|
| `MSX2.ROM` | 32K | 主 BIOS(開機 / BASIC / 基本 I/O) |
| `MSX2EXT.ROM` | 16K | sub/ext BIOS(MSX2 VDP / SCREEN 5-8 / 擴充呼叫) |
| `DISK.ROM` | 16K | 磁碟 BIOS(FDC 驅動,讀 .dsk 開機) |
| `KANJI.ROM` | 128K | 日文漢字字型(日版遊戲文字;不影響 tile 但避免卡開機) |

**來源(GitHub,已下載驗證)**:`github.com/archtaurus/RetroPieBIOS/BIOS/`
raw:`https://raw.githubusercontent.com/archtaurus/RetroPieBIOS/master/BIOS/<檔>`
sha1:MSX2=6103b39f… EXT=5c1f9c7f… DISK=032cb1c1… KANJI=84a645be…
→ 放 `~/.openMSX/share/systemroms/`(或自訂機器 config 以 `<filename>` 直接引用)。

## 3. 建 openMSX(Ubuntu 不在 apt → 源碼編)
```bash
apt-get install -y g++ make python3 pkg-config \
  libsdl2-dev libsdl2-ttf-dev libpng-dev tcl-dev \
  libogg-dev libtheora-dev libvorbis-dev libglew-dev
git clone --depth 1 https://github.com/openMSX/openMSX.git
cd openMSX && ./configure && make -j$(nproc)
# 產物:derived/*/bin/openmsx
```
> 替代:flatpak `org.openmsx.openMSX`(若有 flatpak)。

## 4. 自訂「MSX2 + 磁碟」機器設定
`~/.openMSX/share/machines/U1MSX2.xml`:以 `<filename>` 引用上面 4 顆 ROM,
加 slot:主BIOS(0-0)、ext+disk(3-x)、kanji,FDC=WD2793/MB8877 + 1 台 drive、128K RAM、VDP=V9938。
(可複製 openMSX 內建 `Philips_NMS_8245` 機器設定,把 rom sha1 換成 filename 引我們的檔。)

## 5. Headless 跑 + dump VRAM
```bash
openmsx -machine U1MSX2 -diska "u1.dsk" \
  -script dump.tcl \
  ::: SDL_VIDEODRIVER=dummy / 或 xvfb
```
`dump.tcl`(Tcl 主控台):
```tcl
after time 8 "dump_vram"   ;# 等開機+載入(秒)
proc dump_vram {} {
  debug save_to_file "physical VRAM" 0 0x20000 /work/vram.bin
  # 也存畫面比對:
  set_screenshot ...
  exit
}
```
- ⚠ 進 overworld 需建角輸入 → Tcl `keymatrixdown`/`type` 自動敲鍵跑過 title/建角,
  再 dump(tileset 此時已在 VRAM)。或先 dump title 畫面驗證 BIOS/disk 開機成功。

## 6. 用 VRAM 解 MSXTILES
- 比對 VRAM 的 pattern generator / color table / palette registers(V9938 palette)實際佈局,
  反推 MSXTILES.BIN 的對應 → 寫正確解碼器 → 輸出 832×16 PNG(同 E1 流程)。

## 狀態
- [x] BIOS 下載驗證(MSX2/EXT/DISK/KANJI)
- [ ] 編 openMSX
- [ ] 自訂機器 config
- [ ] headless 開機 + dump VRAM
- [ ] 解 MSXTILES → PNG 包

## ★ 實機 ground truth(2026-06-26,openMSX dump)
- 模式 **SCREEN 7**;palette **8 色 GRB**:0黑 1綠 2紅 3黃 4藍 5青 6洋紅 7白(index=B<<2|R<<1|G)。
- 這是 MSXTILES 解碼的正確 palette(先前靜態猜測全錯)。剩:到 overworld dump VRAM 比對佈局。
