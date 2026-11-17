"""D4 oracle of measurement package 5: energy-weighted spectral centroid and spread re-implemented in numpy
(symmetric Hann of 2048 samples, hop 256, frames while k*H + N <= length, power spectrum) on the note WAVs of
`cymatica_game --cacp-run timbre_descriptors`, compared with harmonic_centroid * f0 and harmonic_spread * f0.
  tools/pyenv/.venv/Scripts/python.exe -I <this file> artifacts/cacp/timbre_descriptors_v1 artifacts/cacp/timbre_descriptors_v1_manifest.json
Tolerance (pre-registered): +-2 % per note.
"""
import json
import sys
from pathlib import Path

import numpy as np

FS, N, H = 48000, 2048, 256
d, m = Path(sys.argv[1]), json.load(open(sys.argv[2]))['measures']
win = np.hanning(N)
freqs = np.arange(N // 2 + 1) * FS / N
worst = 0.0
for role, notes in m['notes'].items():
    for i, (note, f0) in enumerate(zip(notes, m['pitches_hz'])):
        raw = (d / f'{role}_{i}.wav').read_bytes()
        x = np.frombuffer(raw[44:], dtype='<f4').astype(np.float64)[0::2]
        w_sum = c_sum = s_sum = 0.0
        for k in range(0, (len(x) - N) // H + 1):
            p = np.abs(np.fft.rfft(x[k * H:k * H + N] * win)) ** 2
            e = p.sum()
            if e <= 0:
                continue
            c = (p * freqs).sum() / e
            s = np.sqrt((p * (freqs - c) ** 2).sum() / e)
            w_sum += e
            c_sum += e * c
            s_sum += e * s
        c, s = c_sum / w_sum, s_sum / w_sum
        rc = abs(c - note['harmonic_centroid'] * f0) / c
        rs = abs(s - note['harmonic_spread'] * f0) / s
        worst = max(worst, rc, rs)
        print(f'{role:8s} {f0:7.2f} Hz centroid numpy {c:8.2f} cymatica {note["harmonic_centroid"] * f0:8.2f} '
              f'spread numpy {s:8.2f} cymatica {note["harmonic_spread"] * f0:8.2f}')
print(f'D4: worst relative difference {worst:.2e} ->', 'AGREE' if worst <= 0.02 else 'DISAGREE')
