"""Measures and selection rule of docs/research/M2_5_CP3_I1_I2_Kick_Preregistration.md.

Usage: python -I kick_presence.py --clips artifacts/cacp/kick_presence_ab_v1 --plan <fixture_intents_seed1337.csv>
The plan CSV comes from `cymatica_game --cacp-run plan_export` of the same build.
"""
import argparse
import csv
import json
from pathlib import Path

import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, lfilter, sosfiltfilt

FS = 48000
K1 = ([1.53512485958697, -2.69169618940638, 1.19839281085285], [1.0, -1.69065929318241, 0.73248077421585])
K2 = ([1.0, -2.0, 1.0], [1.0, -1.99004745483398, 0.99007225036621])


def load(p):
    fs, x = wavfile.read(p)
    assert fs == FS
    return x.astype(np.float64) / 32768.0


def kweight(x):
    return lfilter(*K2, lfilter(*K1, x, axis=0), axis=0)


def band(x, lo, hi):
    return sosfiltfilt(butter(4, [lo, hi], btype='band', fs=FS, output='sos'), x, axis=0)


def win_energy(x, onsets, a, b):
    c = np.concatenate([[0.0], np.cumsum((x ** 2).sum(axis=1))])
    i, j = onsets + int(a * FS), np.minimum(onsets + int(b * FS), len(x))
    return (c[j] - c[i]) / np.maximum(j - i, 1)


def db(v):
    return 10 * np.log10(np.maximum(v, 1e-14))


def q100(pulso, onsets):
    n = int(0.120 * FS)
    low = tot = 0.0
    f = np.fft.rfftfreq(n, 1 / FS)
    for o in onsets:
        s = pulso[o:o + n].mean(axis=1)
        if len(s) < n:
            continue
        p = np.abs(np.fft.rfft(s)) ** 2
        low += p[f < 100].sum()
        tot += p.sum()
    return low / tot


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--clips', required=True)
    ap.add_argument('--plan', required=True)
    a = ap.parse_args()
    d = Path(a.clips)
    man = json.loads((d / 'kick_presence_ab_v1_manifest.json').read_text())
    rows = [r for r in csv.DictReader(open(a.plan)) if r['role'] == '0']
    ons = {s: np.array(sorted(int(r['sample_frame']) for r in rows if int(r['section']) == s)) for s in range(3)}
    trama, vettore = load(d / 'base_trama.wav'), load(d / 'base_vettore.wav')
    res = {}
    for v in man['variants']:
        t = v['tag']
        pulso, corpo, mix = (load(d / f'{t}_{k}.wav') for k in ('pulso', 'corpo', 'mix'))
        n = min(map(len, (pulso, corpo, trama, vettore)))
        others = corpo[:n] + trama[:n] + vettore[:n]
        kp = kweight(pulso)
        bp, bo = band(pulso[:n], 40, 160), band(others, 40, 160)
        cp, co = band(pulso[:n], 2000, 5000), band(others, 2000, 5000)
        r = {k: v[k] for k in ('mix_integrated_lufs', 'true_peak_dbtp', 'max_gain_reduction_db')}
        r['mix_energy_above_6k_db'] = float(db((sosfiltfilt(butter(4, 6000, btype='high', fs=FS, output='sos'), mix, axis=0) ** 2).mean()))
        for s in range(3):
            o = ons[s]
            r['ABC'[s]] = {'onsets': int(len(o)),
                           'E_K_db': float(db(win_energy(kp, o, 0, 0.120).mean())),
                           'Q100': float(q100(pulso, o)),
                           'P_body_db': float(np.median(db(win_energy(bp, o, 0.020, 0.120)) - db(win_energy(bo, o, 0.020, 0.120)))),
                           'P_click_db': float(np.median(db(win_energy(cp, o, 0, 0.015)) - db(win_energy(co, o, 0, 0.015))))}
        res[t] = r
    b = res['base']
    sel = {}
    elig = []
    for t, r in res.items():
        r['mix_delta_lu'] = r['mix_integrated_lufs'] - b['mix_integrated_lufs']
        r['above_6k_delta_db'] = r['mix_energy_above_6k_db'] - b['mix_energy_above_6k_db']
        if t.startswith('pitch'):
            ok = r['max_gain_reduction_db'] <= 0.1 and abs(r['mix_delta_lu']) <= 0.5 and all(r[s]['Q100'] >= 0.75 for s in 'ABC')
            m = min(r[s]['E_K_db'] for s in 'ABC')
            sel[t] = {'eligible': bool(ok), 'min_E_K_db': m}
            if ok:
                elig.append((int(t[5:]), m))
    if elig:
        best = max(m for _, m in elig)
        sel['I1_selected_midi'] = min(p for p, m in elig if best - m <= 0.2)
    else:
        sel['I1_selected_midi'] = None
    sel['I2_selected'] = None
    for t in ('base', 'click3', 'click6', 'click9'):
        r = res[t]
        ok = (all(r[s]['P_click_db'] >= 0 for s in 'ABC') and r['mix_delta_lu'] <= 0.5 and r['max_gain_reduction_db'] <= 0.1
              and r['above_6k_delta_db'] <= 3)
        sel[t] = {'eligible': bool(ok)}
        if ok and sel['I2_selected'] is None:
            sel['I2_selected'] = t
    print(json.dumps({'k5': [man['k5_events_identical'], man['k5_trama_vettore_identical']], 'variants': res, 'selection': sel}, indent=1))


if __name__ == '__main__':
    main()
