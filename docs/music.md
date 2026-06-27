# 各版本背景音樂 — 逆向、原生還原與歷史紀錄

本檔記錄《Ultima I》各平台版本背景音樂(BGM)的**格式逆向查證**與**原生還原步驟**,作為歷史保存紀錄。
方法依 L.CY 指示:**不錄模擬器**,而是「找到音樂資料檔 → 確認格式 → 理解發聲機制 → 自寫合成器原生 render 回 ogg」。

> 設計:遊戲內切 tileset(`F1` / `PageDown`)時,背景音樂**一併換成該平台原版 BGM**(看哪版畫面、聽哪版音樂)。
> 實作見 `main.cpp`(`pngPacks[].music` + `applyMusic()`)、`src/common/Audio.cpp`;缺檔自動退回占位曲 `theme.ogg`。

## 總表

| 平台 | 音樂源 | 發聲晶片 / 格式 | 還原方式 | 狀態 |
|---|---|---|---|---|
| **MSX** | `.dsk` 的 `ULT*.MCP`(9 首) | PSG/FM,Pony Canyon **MCP** 序列 | `render_mcp_music.py` 自寫 FM 合成 | ✅ `ULT1`="LORD BRITISH'S THEME"(41.4s) |
| **PC-98** | `.fdi` 的 `SCORE.DAT`+`VOICE.DAT` | YM2608(OPNA)FM,自訂序列 | `render_pc98_music.py` 自寫 FM 合成 | ✅ 10 首,overworld=song1(52.9s) |
| **FM Towns** | Trilogy CD 的 `*.EUP`+`ULTIMA.FMB` | YM2612 FM,**EUPHONY** 序列 | `render_eup_music.py` 自寫 FM 合成 | ✅ `MAP`=overworld(49.9s);原 ogg 為靜音已重做 |
| **Apple IIgs** | woz resource type `0x8024`(18 個) | Ensoniq 5503 DOC 8-bit 取樣 | 直接解 8-bit PCM | 🟡 **全是 0.1–2.1s 音效(SFX),非 overworld BGM**(見下) |
| **Atari 8-bit** | — | POKEY | — | ❌ **原版無 overworld BGM**(反組譯證據,見下) |
| **DOS(EGA/CGA/VGA)** | — | PC speaker | — | ❌ 原版無獨立 BGM → 占位曲 |

> 三版有完整地圖 BGM(MSX/PC-98/FM Towns),且為**同一作曲團隊** —— 三種格式都共用 `e0`/`e2`/`fa` 控制碼,
> 破解一個就懂下一個。版權音檔不入庫:`assets/music/*.ogg`、`assets/music/**/*.ogg` 已 gitignore(僅占位 `theme.ogg` 入庫)。

---

## 還原步驟(逐平台,可重跑)

前置:版權的原始資料自備並抽出到 `re_work/`(磁碟用 docker mtools 抽,見下),工具在 `tools/`,需 `python3 + numpy + ffmpeg`。

### MSX(`.MCP` → ogg)
```bash
# 1) 從 .dsk(FAT12)抽 ULT*.MCP(docker mtools)
#    mcopy -n -o "z:/ULT1.MCP" ./ULT1.MCP  (drive z = Ultima I MSX .dsk)
# 2) 渲染(ULT1 = Lord British's Theme = overworld)
python3 tools/render_mcp_music.py re_work/msx/mcp/ULT1.MCP /tmp/msx.wav
ffmpeg -y -i /tmp/msx.wav -ac 1 -q:a 5 assets/music/msx.ogg
```

### PC-98(`SCORE.DAT` → ogg)
```bash
# SCORE.DAT/VOICE.DAT 從 .fdi(FAT12,去 4096 header)→ docker mtools 抽
python3 tools/render_pc98_music.py re_work/pc98/SCORE.DAT 1 /tmp/pc98.wav   # 1 = overworld(song index)
ffmpeg -y -i /tmp/pc98.wav -ac 1 -q:a 5 assets/music/pc98.ogg
```

### FM Towns(`.EUP` → ogg)
```bash
# *.EUP + ULTIMA.FMB 來自 Trilogy CD 的 sound 區(u7-cht/fmtowns_work/sound_extract)
python3 tools/render_eup_music.py re_work/fmtowns/MAP.EUP /tmp/fmt.wav      # MAP = overworld
ffmpeg -y -i /tmp/fmt.wav -ac 1 -q:a 5 assets/music/fmtowns.ogg
```

### Apple IIgs(SFX 取樣 → ogg,僅音效)
```bash
# type 0x8024 resource = 8-bit DOC 取樣;header 8 byte 後為 unsigned 8-bit PCM,~26kHz
python3 - <<'PY'
import numpy as np, wave
d=open('re_work/iigs/res/t8024_id10_55562.bin','rb').read()
a=np.frombuffer(d[8:],dtype=np.uint8).astype(float)-128
pcm=np.clip(a*180,-32767,32767).astype('<i2')
w=wave.open('/tmp/iigs.wav','wb'); w.setnchannels(1);w.setsampwidth(2);w.setframerate(26000); w.writeframes(pcm.tobytes()); w.close()
PY
```

---

## 格式逆向查證(歷史紀錄)

### 共通:作曲團隊與控制碼家族
MSX(MCP)、PC-98(SCORE)、FM Towns(EUP)三版音樂出自同一日本團隊,序列格式同源:
皆以 **`e0`/`e2`/`fa` 等控制碼(byte ≥ 0xe0)+ 音符事件**構成,差別只在事件欄位排列與長度。
發聲晶片不同(MSX PSG / PC-98 OPNA / FM Towns OPN2),但本專案統一用**自寫 2-op FM 合成器**
(modulator→carrier + ADSR,`render_pc98_music.py:fm_note`)還原 chiptune FM 質感,完全不跑模擬器。

### MSX `.MCP`(Pony Canyon)
- 檔頭 `M1` magic + 標題(`ULT1.MCP` = `"ULTIMA M1 LORD BRITISH'S THEME"`)+ track 名(`BASS/CHORD`、`MELODY`)。
- 事件區 **4 byte/事件**;`fe fe fe fe` = 音軌分隔。控制碼 byte0 ≥ 0xe0 或 0xfd(4 byte);
  音符 = `[note, gate, step, velocity]`(note=MIDI 音高、step=到下一事件 tick)。3 軌(旋律/低音/和弦)。

### PC-98 `SCORE.DAT`
- 開頭 16-bit LE offset 表 → **10 首歌**;每首開頭 **6 個 channel 指標**(YM2608 6 FM 聲道)。
- 事件 = 控制碼(byte ≥ 0xe0)+ **(note, dur)** 兩 byte。**破解關鍵**:控制碼參數長度不一
  (`e0`/`e2`/`f4`/`fa`=1 參數、`fb`/`fc`=0 參數、`fe`=結束)造成對齊漂移,暴力求出長度表後音高全落合理音域。
- `VOICE.DAT`(1024B)= 32 個 FM 音色(前段名字如 `"piano"` + operator 參數)。

### FM Towns `.EUP`(EUPHONY)
- 檔頭(標題 + 固定音名表)後有 `"EUPHONY "` 簽章 → 其後 **6 byte/事件**。
- `byte0`=status(0x9n note-on / 0x8n note-off,ch=status&0xF)、`byte1`=**step delta tick**
  (全曲累積,sum≈4930 對上 ~53s,**這是破解關鍵**:byte1 是 delta 不是絕對時間)、`byte4`=note、`byte5`=velocity。
- ⚠ 雷:u7-cht 既有 `eup_*.ogg` 經實測解碼為**靜音**(RMS=0,原轉檔失敗),已用自寫渲染重做。

### Apple IIgs(查證:音效而非音樂)
- 資源型別盤點:type `0x0001`×79(壓縮圖)、`0x8024`×18(**rSoundSample**)、`0x8015`×2(文字)、其餘為版權字串/小表。
- `0x8024` 全 18 個解 8-bit DOC PCM(header 8 byte 後),時長 **0.1–2.1s** → 是**音效(SFX)**,非地圖 BGM。
- 反組譯 `ULTIMAI` 程式:toolbox 用 **Sound Toolset(tool 0x08)**,**無 Note Sequencer(0x19)/ Note Synth(0x1d)呼叫**
  → 不走 synthLAB 音序;音樂機制為「程式觸發取樣音」。**未在資源中找到連續 overworld 音樂軌**。
- 結論:overworld BGM 不像 FM 三版有現成序列檔可還原;若存在則需追程式的觸發序列(`0x8013` 疑似 sound 對照表)或在他碟。
  18 個 SFX 已解於 `assets/music/iigs/iigs_idXX.ogg`(參考,未接入為 BGM)。

### Atari 8-bit(查證:原版無 BGM)
- 反組譯各 `.bin`(Atari binary load):掃 POKEY audio 暫存器寫入 `STA $D200–$D208`。
- `OUTMOVE`(overworld)= **0 次** POKEY audio 寫入;僅 `SPAMOVE`(太空戰)7 次(音效)、`DOS`/`DUP`(開機聲)。
- ⇒ Atari U1 overworld **無背景音樂**,符合 1983 原版(Sierra/Origin)「音效為主、無 BGM」史實 → 退回占位曲。
