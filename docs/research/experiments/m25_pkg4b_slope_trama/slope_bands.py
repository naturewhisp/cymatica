"""Package 4b, section 1: comparable spectral slope (third-octave bands, one point per band, 99.2-4000 Hz) with the
G1-G2 controls and the previous method for comparison.
  tools/pyenv/.venv/Scripts/python.exe -I <this file> artifacts/cacp/stem_export_v2 <output.json>
"""
import json
import sys
from pathlib import Path

import numpy as np
from scipy import signal

FS = 48000
CENTRES = 1000 * 2 ** (np.arange(-10, 7) / 3)          # 99.2 .. 4000 Hz, 17 bands
SUBRANGES = {'99_500': (99, 500), '500_2000': (500, 2000), '2000_4000': (2000, 4000.1)}


def psd(x):
    return signal.welch(x, FS, window='hann', nperseg=8192)


def band_levels(f, p, centres):
    lv = []
    for c in centres:
        sel = (f >= c * 2 ** (-1 / 6)) & (f < c * 2 ** (1 / 6))
        lv.append(10 * np.log10(np.mean(p[sel])) if sel.any() and np.mean(p[sel]) > 0 else np.nan)
    return np.array(lv)


def slope_new(x, lo=99, hi=4000.1):
    f, p = psd(x)
    c = CENTRES[(CENTRES >= lo) & (CENTRES <= hi)]
    lv = band_levels(f, p, c)
    ok = np.isfinite(lv)
    if ok.sum() < 2:
        return float('nan')
    return float(np.polyfit(np.log2(c[ok]), lv[ok], 1)[0])


def slope_old(x):
    f, p = psd(x)
    sel = (f >= 50) & (f <= 16000) & (p > 0)
    return float(np.polyfit(np.log2(f[sel]), 10 * np.log10(p[sel]), 1)[0]) if sel.sum() >= 2 else float('nan')


def shaped_noise(gain_db_of_f, seconds=8, seed=1):
    rng = np.random.default_rng(seed)
    n = FS * seconds
    spec = np.fft.rfft(rng.standard_normal(n))
    f = np.fft.rfftfreq(n, 1 / FS)
    g = np.zeros_like(f)
    g[1:] = 10 ** (gain_db_of_f(f[1:]) / 20)
    return np.fft.irfft(spec * g, n)


def read_mono(p):
    raw = Path(p).read_bytes()
    assert raw[:4] == b'RIFF' and int.from_bytes(raw[20:22], 'little') == 3
    x = np.frombuffer(raw[44:], dtype='<f4').astype(np.float64)
    return (x[0::2] + x[1::2]) / 2


d = Path(sys.argv[1])
out = {'controls': {}, 'whole': {}, 'excerpts': {}}
white = shaped_noise(lambda f: 0 * f)
pink = shaped_noise(lambda f: -10 * np.log10(f))           # -3 dB/oct density
curve = shaped_noise(lambda f: np.where(f <= 1000, -3 * np.log2(f / 1000), -12 * np.log2(f / 1000)))
out['controls'] = {'G1_white_new': slope_new(white), 'G1_pink_new': slope_new(pink),
                   'G2_curve_new': slope_new(curve), 'G2_curve_old': slope_old(curve), 'G2_analytic': -5.934}
for name in ['mix', 'pulso', 'corpo', 'trama', 'vettore']:
    x = read_mono(d / f'whole_{name}.wav')
    out['whole'][name] = {'new_99_4000': slope_new(x), 'old_50_16000': slope_old(x),
                          **{f'new_{k}': slope_new(x, *v) for k, v in SUBRANGES.items()}}
for s in ['A', 'B', 'Aprime']:
    x = read_mono(d / f'{s}_t0_mix.wav')
    out['excerpts'][s] = {'new_99_4000': slope_new(x), 'old_50_16000': slope_old(x)}
Path(sys.argv[2]).write_text(json.dumps(out, indent=2), encoding='utf-8')
print(json.dumps(out, indent=1))
