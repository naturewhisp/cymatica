"""Measurement package 4 (docs/research/M2_5_Pkg4_Specific_Loudness_Masking_Preregistration.md): validation L1-L5 and
fixture measures (loudness increment, masking loss per role and Bark band, spectral slope) with MoSQITo ISO 532-1.

Run with the pinned oracle environment, after `cymatica_game --cacp-run stem_export`:
  tools/pyenv/.venv/Scripts/python.exe -I <this file> artifacts/cacp/stem_export_v1 <output.json>
Calibration (declared): a full-scale sine (peak 1.0) is 100 dB SPL; mono = mean of the two channels.
"""
import json
import sys
from pathlib import Path

import numpy as np
from mosqito.sq_metrics import loudness_zwst, loudness_zwtv
from scipy import signal

FS = 48000
P0 = 2e-5
FULL_SCALE_PA = P0 * 10 ** (100 / 20) * np.sqrt(2)   # peak pressure of a full-scale sine at 100 dB SPL
ROLES = ['pulso', 'corpo', 'trama', 'vettore']
SECTIONS = ['A', 'B', 'Aprime']


def tone(f, spl, seconds=1.0):
    t = np.arange(int(FS * seconds)) / FS
    return P0 * 10 ** (spl / 20) * np.sqrt(2) * np.sin(2 * np.pi * f * t)


def pink(spl, seconds=1.0, seed=1):
    rng = np.random.default_rng(seed)
    n = int(FS * seconds)
    spec = np.fft.rfft(rng.standard_normal(n))
    f = np.fft.rfftfreq(n, 1 / FS)
    spec[1:] /= np.sqrt(f[1:])
    spec[0] = 0
    x = np.fft.irfft(spec, n)
    return x / np.sqrt(np.mean(x ** 2)) * P0 * 10 ** (spl / 20)


def nst(x):
    return float(loudness_zwst(x, FS, field_type='free')[0])


def ntv(x):
    n, nspec, bark, _ = loudness_zwtv(x, FS, field_type='free')
    return float(np.mean(n)), np.mean(nspec, axis=1), bark


def slope(x):
    f, p = signal.welch(x, FS, window='hann', nperseg=8192)
    sel = (f >= 50) & (f <= 16000) & (p > 0)
    if sel.sum() < 2:
        return float('nan')   # silent stem: undefined, never 0
    return float(np.polyfit(np.log2(f[sel]), 10 * np.log10(p[sel]), 1)[0])


def read_mono(p):
    # 32-bit float WAV written by stem_export_v1: canonical 44-byte header (format 3, stereo, 48 kHz)
    raw = Path(p).read_bytes()
    assert raw[:4] == b'RIFF' and int.from_bytes(raw[20:22], 'little') == 3 and int.from_bytes(raw[22:24], 'little') == 2
    data = np.frombuffer(raw[44:], dtype='<f4').astype(np.float64)
    return (data[0::2] + data[1::2]) / 2 * FULL_SCALE_PA


def validation():
    out = {}
    out['L1'] = {'tone_1k_40dB_sone': nst(tone(1000, 40)), 'tone_1k_60dB_sone': nst(tone(1000, 60))}
    t = tone(1000, 60)
    nt = nst(t)
    losses = []
    for lvl in (40, 50, 60, 70):
        nz = pink(lvl)
        losses.append(1 - (nst(t + nz) - nst(nz)) / nt)
    out['L2'] = {'noise_db_spl': [40, 50, 60, 70], 'tone_loss': losses}
    a, b = tone(200, 60), tone(5000, 60)
    na, nb, nab = nst(a), nst(b), nst(a + b)
    out['L3'] = {'loss_200': 1 - (nab - nb) / na, 'loss_5000': 1 - (nab - na) / nb}
    rng = np.random.default_rng(3)
    out['L5'] = {'white_db_per_oct': slope(rng.standard_normal(FS * 4)), 'pink_db_per_oct': slope(pink(60, 4, seed=4))}
    return out


def fixture(d):
    res = {}
    for s in SECTIONS:
        mix = read_mono(d / f'{s}_mix.wav')
        n_mix, spec_mix, bark = ntv(mix)
        row = {'mix_sone': n_mix, 'mix_slope_db_per_oct': slope(mix), 'roles': {}}
        for r in ROLES:
            alone = read_mono(d / f'{s}_{r}.wav')
            without = read_mono(d / f'{s}_mix_without_{r}.wav')
            n_alone, spec_alone, _ = ntv(alone)
            n_without, spec_without, _ = ntv(without)
            inc = n_mix - n_without
            band_inc = spec_mix - spec_without
            with np.errstate(divide='ignore', invalid='ignore'):
                band_loss = np.where(spec_alone > 0.01, 1 - band_inc / spec_alone, np.nan)
            row['roles'][r] = {'alone_sone': n_alone, 'increment_sone': inc,
                               'masking_loss': 1 - inc / n_alone if n_alone > 0 else None,
                               'slope_db_per_oct': slope(alone),
                               'band_loss_every_2_bark': [None if np.isnan(v) else float(v) for v in band_loss[::20]],
                               'bark_axis_every_2': [float(v) for v in bark[::20]]}
        res[s] = row
    return res


d = Path(sys.argv[1])
first = {'validation': validation(), 'fixture': fixture(d)}
second = {'validation': validation(), 'fixture': fixture(d)}
first['L4_repeat_identical'] = json.dumps(first['validation'], sort_keys=True) == json.dumps(second['validation'], sort_keys=True) and \
    json.dumps(first['fixture'], sort_keys=True) == json.dumps(second['fixture'], sort_keys=True)
Path(sys.argv[2]).write_text(json.dumps(first, indent=2), encoding='utf-8')
v = first['validation']
print('L1', v['L1'])
print('L2', [round(x, 4) for x in v['L2']['tone_loss']])
print('L3', {k: round(x, 4) for k, x in v['L3'].items()})
print('L5', {k: round(x, 3) for k, x in v['L5'].items()})
print('L4 repeat identical', first['L4_repeat_identical'])
for s, row in first['fixture'].items():
    print(s, 'mix', round(row['mix_sone'], 2), 'sone, slope', round(row['mix_slope_db_per_oct'], 2))
    for r, m in row['roles'].items():
        fmt = lambda v, n: 'undefined' if v is None or not np.isfinite(v) else round(v, n)
        print('   ', r, 'alone', fmt(m['alone_sone'], 2), 'inc', fmt(m['increment_sone'], 2), 'loss', fmt(m['masking_loss'], 3),
              'slope', fmt(m['slope_db_per_oct'], 2))
