"""Measures T1-T7 of docs/research/M2_5_CP3_I5_Transition_Investigation_Preregistration.md (aggregate statistics only).

Usage: python -I transition_investigation.py --reference <wav> --seed34 <wav> --expert <wav> --plan <fixture_intents csv>
"""
import argparse
import csv
import json
import sys
from pathlib import Path

import numpy as np
from scipy.io import wavfile

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'expert_production_analysis'))
import stem_analysis as S  # noqa: E402

FS = 48000
EDGES = np.geomspace(40, 16000, 25)


def load(p):
    fs, x = wavfile.read(p)
    assert fs == FS
    x = x.astype(np.float64) / (32768.0 if x.dtype.kind == 'i' else 1.0)
    return x


def features(x, t0, t1):
    i, j = int(round(t0 * FS)), int(round(t1 * FS))
    seg = x[i:j].mean(axis=1)
    n = 8192
    w = np.hanning(n)
    fr = [np.abs(np.fft.rfft(seg[k:k + n] * w)) ** 2 for k in range(0, len(seg) - n + 1, n // 2)]
    p = np.mean(fr, axis=0)
    f = np.fft.rfftfreq(n, 1 / FS)
    band = np.array([p[(f >= a) & (f < b)].sum() for a, b in zip(EDGES, EDGES[1:])])
    bdb = 10 * np.log10(band + 1e-12)
    ch = np.zeros(12)
    sel = (f >= 80) & (f <= 2000)
    pc = np.round(69 + 12 * np.log2(f[sel] / 440)).astype(int) % 12
    np.add.at(ch, pc, p[sel])
    k = S.lfilter(S.K2_B, S.K2_A, S.lfilter(S.K1_B, S.K1_A, x[i:j], axis=0), axis=0)
    return {'spec': bdb - bdb.mean(), 'chroma': ch, 'level': 10 * np.log10((k ** 2).sum(axis=1).mean() + 1e-12),
            'high': 10 * np.log10(p[f >= 4000].sum() + 1e-12)}


def analyse(x, edges):
    """edges: bar lines in seconds; every interior bar line with 4 bars before and 2 after."""
    out = {}
    cache = {}

    def two(b):
        if b not in cache:
            cache[b] = features(x, edges[b], edges[b + 2])
        return cache[b]
    for b in range(4, len(edges) - 2):
        pre2, pre, post = two(b - 4), two(b - 2), two(b)
        out[b] = {'spectrum': float(np.linalg.norm(post['spec'] - pre['spec'])),
                  'chroma': float(1 - pre['chroma'] @ post['chroma'] / (np.linalg.norm(pre['chroma']) * np.linalg.norm(post['chroma']) + 1e-20)),
                  'level_step_db': float(post['level'] - pre['level']),
                  'prep_level_db': float(pre['level'] - pre2['level']), 'prep_high_db': float(pre['high'] - pre2['high'])}
        out[b]['prepared'] = max(abs(out[b]['prep_level_db']), abs(out[b]['prep_high_db'])) > 1.5
    return out


def rank(out, b, key):
    v = sorted((o[key] for o in out.values()), reverse=True)
    return {'value': out[b][key], 'rank': 1 + sum(x > out[b][key] for x in v), 'of': len(v),
            'percentile': float(np.mean([x < out[b][key] for x in v]) * 100)}


def main():
    ap = argparse.ArgumentParser()
    for a in ('reference', 'seed34', 'expert', 'plan'):
        ap.add_argument('--' + a, required=True)
    a = ap.parse_args()
    bstart = 64.0
    apstart = 64.0 + 32 * 4 * 60 / 140
    ours = [k * 2.0 for k in range(32)] + [bstart + k * 4 * 60 / 140 for k in range(32)] + [apstart + k * 2.0 for k in range(33)]
    res = {}
    for name, p in (('reference', a.reference), ('seed_34', a.seed34)):
        o = analyse(load(p), ours)
        res[name] = {lab: {**o[b], 'spectrum': rank(o, b, 'spectrum'), 'chroma': rank(o, b, 'chroma')} for lab, b in (('A_to_B', 32), ('B_to_Aprime', 64))}
        res[name]['prepared_share_other_barlines'] = float(np.mean([v['prepared'] for b, v in o.items() if b not in (32, 64)]))
    x = load(a.expert)
    bar = 4 * 60 / 162
    ed = list(np.arange(0, len(x) / FS, bar))
    o = analyse(x, ed)
    marks = {t: int(round(t / bar)) for t in (47, 121, 165, 230)}
    res['expert'] = {str(t): {**o[b], 'bar': b, 'spectrum': rank(o, b, 'spectrum'), 'chroma': rank(o, b, 'chroma')} for t, b in marks.items()}
    res['expert']['prepared_share_other_barlines'] = float(np.mean([v['prepared'] for b, v in o.items() if b not in marks.values()]))
    # T7: pad held across A -> B against the notes attacking in the first beat of B
    rows = list(csv.reader(open(a.plan)))[1:]
    off = {r[1]: int(r[3]) for r in rows if r[0] == '#noteoff'}
    notes = [dict(id=r[0], role=int(r[3]), f=int(r[4]), m=int(round(69 + 12 * np.log2(float(r[6]) / 440)))) for r in rows if r[0] != '#noteoff']
    fb = int(bstart * FS)
    held = [n for n in notes if n['id'] in off and n['f'] < fb < off[n['id']]]
    first = [n for n in notes if fb <= n['f'] < fb + int(60 / 140 * FS) and n['role'] in (1, 2, 3)]
    res['T7'] = {'held_midi': [n['m'] for n in held], 'attacks_midi_role': [(n['m'], n['role']) for n in first],
                 'semitone_pairs': sum((h['m'] - n['m']) % 12 in (1, 11) for h in held for n in first)}
    r = res['reference']
    res['outcome'] = {'T1': r['A_to_B']['chroma']['rank'] <= 3, 'T2': r['A_to_B']['spectrum']['percentile'] <= 95,
                      'T3': not any(res[n][b]['prepared'] for n in ('reference', 'seed_34') for b in ('A_to_B', 'B_to_Aprime')),
                      'T4': sum(res['expert'][str(t)]['prepared'] for t in marks) >= 3,
                      'T5': res['expert']['prepared_share_other_barlines'] < 0.5,
                      'T6': abs(r['A_to_B']['level_step_db']) >= 1, 'T7': res['T7']['semitone_pairs'] >= 1}
    print(json.dumps(res, indent=1, default=lambda v: v.item()))


if __name__ == '__main__':
    main()
