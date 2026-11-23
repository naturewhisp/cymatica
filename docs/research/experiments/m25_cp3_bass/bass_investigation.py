"""Measures of docs/research/M2_5_CP3_I3_Bass_Investigation_Preregistration.md (aggregate statistics only).

Usage: python -I bass_investigation.py --midi <dir> --gift <decoded wav dir> --clips artifacts/cacp/kick_presence_ab_v1
       --plan artifacts/cacp/plan_export_v1/fixture_intents_seed1337.csv
"""
import argparse
import csv
import json
import sys
from pathlib import Path

import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, lfilter, sosfilt, sosfiltfilt

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'expert_production_analysis'))
import midi_analysis as M  # noqa: E402
import stem_analysis as S  # noqa: E402

FS = 48000


def db(v):
    return 10 * np.log10(np.maximum(v, 1e-14))


def part_a(midi, gift):
    ks = M.read_smf(midi / 'Kick & Snare.mid')
    out = {}
    for name in ('808_Bass', 'SubBass'):
        m = M.read_smf(midi / f'{name}.mid')
        ppq = m['ppq']
        bar, tol, six = 4 * ppq, ppq // 8, ppq // 4
        kicks = {}
        for n in ks['notes']:
            if n['pitch'] == 36:
                kicks.setdefault(n['tick'] // bar, []).append(n['tick'] % bar)
        on = sorted({n['tick'] for n in m['notes'] if n['tick'] // bar in kicks})

        def share(shift):
            hit = 0
            for t in on:
                p = t % bar
                d = [abs(p - (k + shift * six) % bar) for k in kicks[t // bar]]
                hit += min(min(x, bar - x) for x in d) <= tol
            return hit / len(on) if on else None
        seq = sorted(m['notes'], key=lambda n: (n['tick'], -n['pitch']))
        low = {}
        for n in seq:
            low[n['tick']] = min(low.get(n['tick'], 999), n['pitch'])
        p = [low[t] for t in sorted(low)]
        steps = np.abs(np.diff(p))
        grid = [round((t % bar) / six) % 16 for t in sorted(low)]
        out[name] = {'onsets_in_kick_bars': len(on), 'A1_coincidence': share(0),
                     'A2_shifted_mean': float(np.mean([share(k) for k in range(1, 16)])),
                     'A4_steps_0_2': float(np.mean(steps <= 2)), 'A4_leaps_7plus': float(np.mean(steps >= 7)),
                     'A5_quarters_share': float(np.mean([g % 4 == 0 for g in grid])),
                     'onset_grid16': {int(k): int(v) for k, v in zip(*np.unique(grid, return_counts=True))}}
    for stem in ('808 Bass', 'Sub Bass'):
        fs, x = S.load(gift / f'{stem}.wav')
        x = x.mean(axis=1)
        n = int(0.2 * fs)
        f = np.fft.rfftfreq(8 * n, 1 / fs)
        sel = (f >= 25) & (f <= 150)
        pk, w = [], []
        win = np.hanning(n)
        for i in range(0, len(x) - n, n):
            sp = np.abs(np.fft.rfft(x[i:i + n] * win, 8 * n)) ** 2
            e = sp[sel].sum()
            pk.append(f[sel][sp[sel].argmax()])
            w.append(e)
        pk, w = np.array(pk), np.array(w)
        o = np.argsort(pk)
        c = np.cumsum(w[o]) / w.sum()
        out[f'A3_fundamental_hz_{stem}'] = {q: float(pk[o][np.searchsorted(c, v)]) for q, v in (('q25', .25), ('median', .5), ('q75', .75))}
    return out


def load(p):
    return wavfile.read(p)[1].astype(np.float64) / 32768.0


def bp(x, lo, hi):
    return sosfiltfilt(butter(4, [lo, hi], btype='band', fs=FS, output='sos'), x, axis=0)


def kw(x):
    return lfilter(S.K2_B, S.K2_A, lfilter(S.K1_B, S.K1_A, x, axis=0), axis=0)


def main():
    ap = argparse.ArgumentParser()
    for a in ('midi', 'gift', 'clips', 'plan'):
        ap.add_argument('--' + a, required=True)
    a = ap.parse_args()
    out = {'A': part_a(Path(a.midi), Path(a.gift))}
    clips = Path(a.clips)
    # Frames of the section boundaries: A 120 BPM (32 bars), B 140 BPM (32), A' 120 BPM (32)
    bounds = [0, 3072000, 3072000 + int(32 * 4 * 60 * FS / 140), 0]
    bounds[3] = bounds[2] + 3072000
    beat = [FS // 2, 60 * FS / 140, FS // 2]
    pcs = [[2, 7, 9, 2], [7, 3, 5, 7], [2, 7, 9, 2]]  # roots per bar (reference plan)
    rows = list(csv.DictReader(open(a.plan)))
    kicks = np.array(sorted(int(r['sample_frame']) for r in rows if r['role'] == '0'))
    corpo, trama, vett = (load(clips / f'base_{r}.wav') for r in ('corpo', 'trama', 'vettore'))
    out['B'], out['C'] = {}, {}
    for kick in ('base', 'combo36click3'):
        pulso = load(clips / f'{kick}_pulso.wav')
        corpo = load(clips / f'{kick}_corpo.wav')
        rb = {}
        low = {n: bp(x, 30, 120) for n, x in (('pulso', pulso), ('corpo', corpo), ('trama', trama), ('vettore', vett))}
        wide = bp(pulso, 30, 160)
        for s in range(3):
            i, j = bounds[s], min(bounds[s + 1], len(pulso))
            e = {n: (x[i:j] ** 2).sum() for n, x in low.items()}
            n10 = FS // 100
            w = wide[i:j]
            w = (w[:len(w) // n10 * n10] ** 2).sum(axis=1).reshape(-1, n10).mean(axis=1)
            rb['ABC'[s]] = {'B1_kick_share_30_120': float(e['pulso'] / sum(e.values())),
                            'B2_free_time_share': float(np.mean(db(w) < db(w).max() - 20))}
        out['B'][kick] = rb
        res = {}
        kb = bp(pulso, 40, 160)
        for reg, lo in (('low', 28), ('high', 40)):
            for sep in ('S0', 'S1', 'S2', 'S3', 'S4'):
                per = {}
                for s in range(3):
                    i, j = bounds[s], bounds[s + 1]
                    n = min(j, len(pulso)) - i
                    k_here = kicks[(kicks >= i) & (kicks < i + n)]

                    def synth(alternate):
                        y = np.zeros(n)
                        eighth = beat[s] / 2
                        tol = beat[s] / 8
                        for b in range(32):
                            midi = lo + (pcs[s][b % 4] - lo) % 12
                            f0 = 440 * 2 ** ((midi - 69) / 12)
                            for q in range(8):
                                t0 = int((b * 8 + q) * eighth)
                                end = t0 + int(0.8 * eighth)
                                if alternate and len(k_here) and np.abs(k_here - i - t0).min() <= tol:
                                    t0 += int(0.060 * FS)
                                m = min(end, n) - t0
                                if m <= 1:
                                    continue
                                t = np.arange(m) / FS
                                v = sum(g * np.sin(2 * np.pi * h * f0 * t) for h, g in ((1, 1.0), (2, 10 ** (-6 / 20)), (3, 10 ** (-12 / 20))))
                                env = np.minimum(1, t / 0.005) * np.minimum(1, (m / FS - t) / 0.010)
                                y[t0:t0 + m] += v * env
                        return y
                    ref = synth(False)
                    pw = lambda x: (kw(x) ** 2).mean()  # noqa: E731
                    target = (kw(corpo[i:i + n]) ** 2).sum(axis=1).mean() * 10 ** (-3 / 10)
                    g = np.sqrt(target / (2 * pw(ref)))
                    ref *= g
                    y = ref.copy()
                    if sep == 'S1':
                        y = synth(True) * g
                    if sep in ('S2', 'S3'):
                        depth = {'S2': 6.0, 'S3': 12.0}[sep]
                        att = np.zeros(n)
                        for k in k_here - i:
                            m = min(n - k, int(0.6 * FS))
                            att[k:k + m] = np.maximum(att[k:k + m], depth * np.exp(-np.arange(m) / (0.120 * FS)))
                        y *= 10 ** (-att / 20)
                    if sep == 'S4':
                        y = sosfilt(butter(4, 90, btype='high', fs=FS, output='sos'), y)
                    yb = bp(y, 40, 160)
                    r = []
                    for k in k_here - i:
                        m = int(0.060 * FS)
                        if k + m > n:
                            continue
                        r.append(db((kb[i + k:i + k + m] ** 2).sum()) - db(2 * (yb[k:k + m] ** 2).sum()))
                    per['ABC'[s]] = {'R_db': float(np.median(r)), 'P_lu': float(db(pw(ref)) - db(pw(y))),
                                     'L_db': float(db((bp(y, 30, 120) ** 2).sum()) - db((bp(ref, 30, 120) ** 2).sum()))}
                ok = all(v['R_db'] >= 6 and v['P_lu'] <= 2 and v['L_db'] >= -3 for v in per.values())
                res[f'{reg}_{sep}'] = {'sections': per, 'eligible': bool(ok)}
        sel = next((f'{reg}_{sep}' for sep in ('S0', 'S2', 'S1', 'S3', 'S4') for reg in ('low', 'high') if res[f'{reg}_{sep}']['eligible']), None)
        out['C'][kick] = {'variants': res, 'selected': sel}
    print(json.dumps(out, indent=1))


if __name__ == '__main__':
    main()
