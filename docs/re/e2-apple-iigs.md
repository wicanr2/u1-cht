# E2 Apple IIgs 素材抽取 — woz 解碼牆(2026-06-26)

## 現況
- 素材:`org_game/Ultima I IIgs.woz`(1994 Heineman/Vitesse IIgs 移植;3.5" ProDOS;`write_protected=0`)。
- **AppleCommander 13 讀 woz 直接回 `null`**(不支援 woz bit-stream)。
- `wozardry.py`(a2-4am)只編輯 WOZ metadata / 讀 track bitstream,**不解碼成邏輯 512B 磁區**。
- Linux 無現成 woz→ProDOS CLI(Applesauce 是 Mac GUI)。

## 牆 + 解法
woz 存的是 flux/bit-stream;3.5" Apple 用 **GCR 6-and-2** 編碼。要取 ProDOS 檔需:
1. 讀 WOZ TRKS bitstream(wozardry 可)。
2. 自寫 **3.5" GCR 解碼器**(找 sync → address field → data field 512B GCR→nibble→byte)→ 組 ProDOS 800K image。
3. AppleCommander 讀 ProDOS image → 抽圖檔。
4. 解 IIgs 圖格式(320 mode,16 色 + SCB/palette)→ 對齊 engine 52 槽 PNG。

工程量:GCR 解碼器 ~150 行 + IIgs 圖解碼。屬 focused 子任務。

## 替代順序建議
- E3 MSX(`.dsk`)、E4 PC-98(`.fdi`)是**標準磁區映像**(非 flux),抽檔較直接 → 可先做。
- E1 FM Towns 已是完整可參考範例(`docs/re/fmtowns-u1-graphics.md`)。
