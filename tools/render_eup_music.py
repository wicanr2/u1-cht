#!/usr/bin/env python3
"""把 FM Towns《Ultima I》的 EUPHONY(.EUP)序列原生 render 成 WAV(自包含 2-op FM,不跑模擬器)。

EUP 格式(由 RE):檔頭含標題 + 固定音名表;音樂事件在 `"EUPHONY "` 簽章後,**6 byte/事件**:
  byte0 = status(0x9n=note on / 0x8n=note off,ch = status & 0x0F);
  byte1 = **step time(delta tick,事件間隔)** —— 全曲累積(sum≈4930 對上 ~53s);
  byte4 = note(MIDI 音高);byte5 = velocity。note-off(note 欄為 0)結束該聲道當前音符。
發聲 = FM:沿用 render_pc98_music 的 2-op FM 合成(modulator→carrier+ADSR)。

用法:render_eup_music.py <song.EUP> <out.wav>
"""
import sys, wave, importlib.util, os
import numpy as np

_r = importlib.util.spec_from_file_location('r', os.path.join(os.path.dirname(__file__), 'render_pc98_music.py'))
pc98 = importlib.util.module_from_spec(_r); _r.loader.exec_module(pc98)

SR = pc98.SR
TICK_S = 0.0108         # tick→秒(總 ~4930 tick 對上 ~53s)

def find_events(d):
    sig = d.find(b'EUPHONY ')
    base = sig + 8
    # 對齊到 byte0 為 status(0x8n/0x9n/0xf*/0x00)的相位
    for ph in range(12):
        ev0 = base + ph
        ok = tot = 0
        i = ev0
        while i + 6 <= len(d) and tot < 40:
            s = d[i]
            if (s & 0xf0) in (0x80, 0x90) or s in (0x00, 0xff, 0xf2):
                ok += 1
            tot += 1; i += 6
        if ok >= 38:
            return ev0
    return base

def parse(d):
    """回傳 {channel: [(start_tick, note, vel, dur_tick), ...]}。
    時間以 byte[1] 為 delta 全曲累積;note-on 開音、note-off(同聲道)關音 → 算出時長。"""
    ev0 = find_events(d)
    chans = {}
    open_note = {}      # ch → (start_tick, note, vel)
    t = 0
    i = ev0
    while i + 6 <= len(d):
        e = d[i:i+6]; i += 6
        if e[0] == 0xff:
            break
        t += e[1]
        st = e[0]; hi = st & 0xf0; ch = st & 0x0f
        if hi == 0x90 and e[5] > 0:                     # note on
            if ch in open_note:                         # 前一音未關 → 先收尾
                s0, n0, v0 = open_note.pop(ch)
                chans.setdefault(ch, []).append((s0, n0, v0, max(t - s0, 1)))
            open_note[ch] = (t, e[4], e[5])
        elif hi == 0x80 and ch in open_note:            # note off
            s0, n0, v0 = open_note.pop(ch)
            chans.setdefault(ch, []).append((s0, n0, v0, max(t - s0, 1)))
    for ch, (s0, n0, v0) in open_note.items():
        chans.setdefault(ch, []).append((s0, n0, v0, 12))
    return chans

def render(path):
    d = open(path, 'rb').read()
    chans = parse(d)
    maxt = max((t + dur for ev in chans.values() for (t, n, v, dur) in ev), default=1)
    buf = np.zeros(int((maxt * TICK_S + 2) * SR))
    for ch, ev in chans.items():
        for (t, note, vel, dur) in ev:
            dur_s = max(dur * TICK_S, 0.05)
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
