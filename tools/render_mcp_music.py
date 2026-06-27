#!/usr/bin/env python3
"""把 MSX 版《Ultima I》(Pony Canyon 1986)的 `.MCP` 音樂原生 render 成 WAV(自包含 FM 合成)。

MCP = Pony Canyon 的音樂格式(與 PC-98 SCORE / FM Towns EUP 同作曲團隊,共用 e0/e2/fa 控制碼)。
格式(由 RE):檔頭 "M1" + 標題(如 `ULT1.MCP` = "LORD BRITISH'S THEME")+ track 名(BASS/CHORD、MELODY)。
事件區 **4 byte/事件**;`fe fe fe fe` = 音軌分隔。
  控制碼:byte0 ≥ 0xe0 或 0xfd(4 byte,跳過)。
  音符:`[note, gate, step, velocity]` —— note=MIDI 音高、gate=音長 tick、step=到下一事件 tick、vel=力度。
發聲:MSX 為 PSG/FM-PAC;這裡沿用 2-op FM 合成(render_pc98_music)近似 chiptune 質感,不跑模擬器。

用法:render_mcp_music.py <song.MCP> <out.wav>
"""
import sys, wave, re, importlib.util, os
import numpy as np

_r = importlib.util.spec_from_file_location('r', os.path.join(os.path.dirname(__file__), 'render_pc98_music.py'))
pc98 = importlib.util.module_from_spec(_r); _r.loader.exec_module(pc98)
SR = pc98.SR
TICK_S = 0.0086

def parse(d):
    bounds = [m.start() for m in re.finditer(b'\xfe\xfe\xfe\xfe', d)]
    tracks = []
    for k, st in enumerate(bounds):
        en = bounds[k+1] if k+1 < len(bounds) else len(d)
        i = st + 4; notes = []; t = 0
        while i + 4 <= en:
            b = d[i:i+4]
            if b == b'\xfe\xfe\xfe\xfe':
                break
            if b[0] >= 0xe0 or b[0] == 0xfd:        # 控制碼
                i += 4; continue
            note, gate, step, vel = b
            if 12 <= note <= 108 and vel > 0:
                notes.append((t, note, max(gate, 1), vel))
            t += step; i += 4
        if len(notes) > 4:
            tracks.append(notes)
    return tracks

def render(path):
    d = open(path, 'rb').read()
    tracks = parse(d)
    maxt = max((t + g for tr in tracks for (t, n, g, v) in tr), default=1)
    buf = np.zeros(int((maxt * TICK_S + 2) * SR))
    for tr in tracks:
        for k, (t, note, gate, vel) in enumerate(tr):
            nxt = tr[k+1][0] if k + 1 < len(tr) else t + gate
            dur_s = max(min(gate, nxt - t) * TICK_S, 0.05)
            sig = pc98.fm_note(pc98.midi_freq(note), dur_s * 0.95, vel / 127.0)
            off = int(t * TICK_S * SR)
            buf[off:off+len(sig)] += sig
    peak = np.max(np.abs(buf)) or 1.0
    return buf / peak * 0.85

def main():
    inp, outp = sys.argv[1], sys.argv[2]
    buf = render(inp)
    pcm = (buf * 32767).astype('<i2')
    w = wave.open(outp, 'wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
    w.writeframes(pcm.tobytes()); w.close()
    print(f"wrote {outp}  ({len(buf)/SR:.1f}s)")

if __name__ == '__main__':
    main()
