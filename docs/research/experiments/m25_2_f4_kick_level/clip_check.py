"""Sanity check of the F4a clips (pure Python): duration, peak, RMS, low-band share (one-pole 150 Hz), full-scale
samples; PNG envelopes of section A, aligned +0 dB (top, blue) and +9 dB (bottom, red), first 4 s."""
import array
import math
import struct
import sys
import wave
import zlib
from pathlib import Path

d = Path(sys.argv[1])
out_png = Path(sys.argv[2])


def read(p):
    with wave.open(str(p), 'rb') as w:
        a = array.array('h')
        a.frombytes(w.readframes(w.getnframes()))
        return a, w.getframerate()


print(f"{'clip':34s} {'dur_s':>6s} {'peak_dB':>8s} {'rms_dB':>7s} {'low<150':>8s} {'full16':>6s}")
for p in sorted(d.glob('*.wav')):
    a, fs = read(p)
    n = len(a) // 2
    alpha = 1 - math.exp(-2 * math.pi * 150 / fs)
    lp = e = el = 0.0
    pk = full = 0
    for i in range(n):
        m = (a[2 * i] + a[2 * i + 1]) / 65536.0
        lp += alpha * (m - lp)
        e += m * m
        el += lp * lp
        v = max(abs(a[2 * i]), abs(a[2 * i + 1]))
        pk = max(pk, v)
        full += v >= 32767
    rms = math.sqrt(e / n)
    print(f"{p.name:34s} {n/fs:6.2f} {20*math.log10(pk/32768):8.2f} {20*math.log10(rms):7.2f} {el/e:8.3f} {full:6d}")

W, H = 800, 300
rows = [[(255, 255, 255)] * W for _ in range(H)]
for row, name in enumerate(['f4a_A_+00dB_aligned.wav', 'f4a_A_+09dB_aligned.wav']):
    a, fs = read(d / name)
    hop = (4 * fs) // W
    y0 = row * 150 + 140
    for i in range(W):
        v = max(abs(a[2 * k] + a[2 * k + 1]) / 65536.0 for k in range(i * hop, (i + 1) * hop))
        h = int(min(v / 0.5, 1.0) * 130)
        for y in range(y0 - h, y0):
            rows[y][i] = (40, 90, 200) if row == 0 else (200, 70, 40)
    rows[y0] = [(0, 0, 0)] * W


def chunk(t, data):
    return struct.pack('>I', len(data)) + t + data + struct.pack('>I', zlib.crc32(t + data) & 0xffffffff)


raw = b''.join(b'\x00' + bytes(c for px in r for c in px) for r in rows)
hdr = struct.pack('>IIBBBBB', W, H, 8, 2, 0, 0, 0)
out_png.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', hdr) + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))
print('png', out_png)
