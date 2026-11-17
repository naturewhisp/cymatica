"""C5 oracle of measurement package 3: scipy.signal.gammatone (FIR variant, 4th order) + numpy ILD and normalised
cross-correlation on the WAVs written by `cymatica_game --cacp-run binaural` (artifacts/cacp/binaural_v1).
Run with the pinned oracle environment:
  tools/pyenv/.venv/Scripts/python.exe -I <this file> artifacts/cacp/binaural_v1 artifacts/cacp/binaural_v1_manifest.json
Tolerances (pre-registered): broadband ILD +-0.3 dB, ITD +-1 sample.
"""
import json
import sys
import wave
from pathlib import Path

import numpy as np
from scipy import signal

FS = 48000
d, manifest = Path(sys.argv[1]), json.load(open(sys.argv[2]))['measures']


def centres(n=24, lo=100.0, hi=8000.0):
    e = lambda f: 21.4 * np.log10(4.37e-3 * f + 1.0)
    inv = lambda x: (10 ** (x / 21.4) - 1.0) / 4.37e-3
    return inv(np.linspace(e(lo), e(hi), n))


def read(p):
    with wave.open(str(p)) as w:
        x = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').astype(np.float64) / 32768.0
    return x[0::2], x[1::2]


def measure(l, r):
    ilds, itds, es = [], [], []
    for fc in centres():
        # FIR variant: scipy's IIR design of the same gammatone puts poles outside the unit circle below about
        # 250 Hz at 48 kHz (|p| up to 1.0095), in direct form and in second-order sections alike
        b, _ = signal.gammatone(fc, 'fir', fs=FS)
        bl, br = signal.fftconvolve(l, b), signal.fftconvolve(r, b)
        eL, eR = np.sum(bl ** 2), np.sum(br ** 2)
        best, bestk = -2.0, 0
        for k in range(-48, 49):
            if k >= 0:
                x, y = bl[:len(bl) - k], br[k:]
            else:
                x, y = bl[-k:], br[:len(br) + k]
            rho = np.dot(x, y) / np.sqrt(np.dot(x, x) * np.dot(y, y))
            if rho > best:
                best, bestk = rho, k
        ilds.append(10 * np.log10(eL / eR))
        es.append(eL + eR)
        if fc < 1500.0:
            itds.append(bestk)
    es = np.array(es)
    return float(np.sum(es * np.array(ilds)) / np.sum(es)), int(sorted(itds)[len(itds) // 2])


ok = True
for pan, ours in zip(manifest['C1']['pan_db'], manifest['C1']['measured_ild_db']):
    ild, _ = measure(*read(d / f'c1_pan_{int(pan)}dB.wav'))
    agree = abs(ild - ours) <= 0.3
    ok &= agree
    print(f'C1 pan {pan:+.0f} dB: scipy ILD {ild:+.3f}, cymatica {ours:+.3f} -> {"AGREE" if agree else "DISAGREE"}')
for dly, ours in zip(manifest['C2']['delay_frames'], manifest['C2']['measured_itd_frames']):
    _, itd = measure(*read(d / f'c2_itd_{dly}.wav'))
    agree = abs(itd - ours) <= 1
    ok &= agree
    print(f'C2 delay {dly:+d}: scipy ITD {itd:+d}, cymatica {ours:+d} -> {"AGREE" if agree else "DISAGREE"}')
ild, _ = measure(*read(d / 'c3_decorrelated.wav'))
agree = abs(ild - manifest['C3']['ild_db']) <= 0.3
ok &= agree
print(f'C3 decorrelated: scipy ILD {ild:+.3f}, cymatica {manifest["C3"]["ild_db"]:+.3f} -> {"AGREE" if agree else "DISAGREE"}')
print('C5:', 'AGREE' if ok else 'DISAGREE')
