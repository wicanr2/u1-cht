#!/usr/bin/env python3
"""把 PC-98《Ultima I》的 SCORE.DAT 序列原生 render 成 WAV(自包含 2-op FM 合成,不跑模擬器)。

格式(由 RE,見 docs/music.md):
  SCORE.DAT:開頭 16-bit LE offset 表(最多 10 首歌)。每首歌開頭 6 個 16-bit channel 指標
  (相對歌起點,對應 YM2608 6 個 FM 聲道)。channel 資料 = 控制碼(byte ≥ 0xe0,各帶 1 參數)
  + (duration, note) 對(note = MIDI 音高,如 0x3c=C4;duration = tick,24 的倍數)。
  控制碼:0xe0=設音色 index、0xfa=tempo、0xfe=channel 結束。

發聲機制 = FM:這裡用 2-operator FM(modulator→carrier)+ ADSR 包絡近似 YM2608 的 FM 音色
(非取樣 soundfont,保留 chiptune FM 質感)。各 channel 單音,混音後正規化。

用法:render_pc98_music.py <SCORE.DAT> <song_index> <out.wav>
"""
import sys, struct, wave
import numpy as np

SR = 44100
PPQN = 48           # 48 tick = 四分音符(24=八分,192=全音符)
BPM = 132           # 預設速度(0xfa tempo 細節未定,取聽感合理值)
TICK_S = (60.0 / BPM) / PPQN

def midi_freq(n):
    return 440.0 * 2.0 ** ((n - 69) / 12.0)

def adsr(n, a, d, s, r):
    env = np.ones(n)
    ai = min(int(a * SR), n)
    di = min(int(d * SR), n - ai)
    ri = min(int(r * SR), n)
    if ai > 0: env[:ai] = np.linspace(0, 1, ai)
    if di > 0: env[ai:ai+di] = np.linspace(1, s, di)
    env[ai+di:] = s
    if ri > 0: env[-ri:] *= np.linspace(1, 0, ri)
    return env

def fm_note(freq, dur_s, vel=1.0):
    n = max(int(dur_s * SR), 1)
    t = np.arange(n) / SR
    # modulator:ratio 2、index 隨包絡衰減(FM 明亮→暗,像撥弦/管樂)
    mod_env = adsr(n, 0.005, 0.12, 0.35, 0.05)
    I = 3.2 * mod_env
    mod = I * np.sin(2 * np.pi * (freq * 2.0) * t)
    car_env = adsr(n, 0.006, 0.10, 0.7, max(0.04, dur_s * 0.2))
    sig = np.sin(2 * np.pi * freq * t + mod) * car_env
    return sig * vel * 0.5

# 控制碼參數長度(由暴力求解,讓 note 合法率最高):
# e0/e2/f4/fa = 1 參數(2 byte);fb/fc = 0 參數(1 byte);fe = 結束。
ZERO_PARAM = {0xfb, 0xfc}

def parse_channel(d, start, end):
    """回傳 [('ctl',cmd,param) | ('note', dur_ticks, midi_note)]。
    事件格式一致為 **(note, dur)** 兩 byte;控制碼 byte ≥ 0xe0(多數 1 參數,部分 0 參數)。
    note 超出合理音域(<12 或 >108)視為休止。"""
    i = start; ev = []
    while i < end - 1:
        b = d[i]
        if b == 0xfe:
            break
        if b >= 0xe0:
            ev.append(('ctl', b, d[i+1] if b not in ZERO_PARAM else 0))
            i += 1 if b in ZERO_PARAM else 2
        else:
            note, dur = d[i], d[i+1]
            ev.append(('note', dur, note if 12 <= note <= 108 else 0))
            i += 2
    return ev

def render_song(d, si):
    songs = [struct.unpack('<H', d[i:i+2])[0] for i in range(0, 0x28, 2)]
    songs = [s for s in songs if s]
    s = songs[si]
    nxt = songs[si+1] if si+1 < len(songs) else len(d)
    ptr = [struct.unpack('<H', d[s+i*2:s+i*2+2])[0] for i in range(6)]
    chans = []
    for ch in range(6):
        st = s + ptr[ch]
        en = (s + ptr[ch+1]) if ch + 1 < 6 else nxt
        chans.append(parse_channel(d, st, en))
    # 總長
    def chan_ticks(ev):
        return sum(e[1] for e in ev if e[0] == 'note')
    total_s = max((chan_ticks(c) for c in chans), default=0) * TICK_S + 1.0
    buf = np.zeros(int(total_s * SR) + SR)
    for ev in chans:
        pos = 0.0
        for kind, a, b in ev:
            if kind == 'ctl':
                continue
            dur_s = a * TICK_S
            if b > 0:   # b==0 視為休止
                note = fm_note(midi_freq(b), dur_s * 0.98)
                off = int(pos * SR)
                buf[off:off+len(note)] += note
            pos += dur_s
    peak = np.max(np.abs(buf)) or 1.0
    return (buf / peak * 0.85)

def main():
    score, si, outp = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    d = open(score, 'rb').read()
    buf = render_song(d, si)
    pcm = (buf * 32767).astype('<i2')
    w = wave.open(outp, 'wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
    w.writeframes(pcm.tobytes()); w.close()
    print(f"wrote {outp}  ({len(buf)/SR:.1f}s, song {si})")

if __name__ == '__main__':
    main()
