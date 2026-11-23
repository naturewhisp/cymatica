"""Melody approach to A' (docs/research/M2_5_CP3_I5c_Melody_Approach_Preregistration.md). Same usage as aprime_entry.py."""
import argparse
import json
import random
import secrets
import sys
from pathlib import Path

import numpy as np
from scipy.io import wavfile

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / 'expert_production_analysis'))
import aprime_entry as E  # noqa: E402
import stem_analysis as S  # noqa: E402
import tempo_ramp as T  # noqa: E402

FS, B0 = E.FS, E.B0
P15 = 16 * 60 / 140  # seconds of phrase 15
EPS = 5e-4


def rel(e):
    return e['f'] / FS - B0


def vett(ev):
    return sorted((e for e in ev if e['role'] == 3), key=lambda e: e['f'])


def switch_point(ev):
    """Index (in phrase 15's Vettore notes) of the first note raised one octave: smallest maximum leap
    from the last note of phrase 14 to the first of A' (I5c bis), latest on ties."""
    v15 = [e for e in vett(ev) if -P15 - EPS <= rel(e) < -EPS]
    first = next(e for e in vett(ev) if rel(e) >= -EPS)
    prev = [e for e in vett(ev) if rel(e) < -P15 - EPS][-1]
    m = [E.midi(e) for e in v15]
    best = None
    for k in range(len(m)):
        q = [E.midi(prev)] + [x + (12 if i >= k else 0) for i, x in enumerate(m)] + [E.midi(first)]
        top = max(abs(b - a) for a, b in zip(q, q[1:]))
        if best is None or top <= best[0]:
            best = (top, k)
    return best[1], best[0], {e['id'] for e in v15[best[1]:]}


def variant(ev, v):
    raised = switch_point(ev)[2] if v == 'u' else set()
    out, changed = [], []
    for e in ev:
        e = dict(e)
        n = m = E.midi(e)
        in16 = -EPS <= rel(e) < 8 - EPS
        if v == 'u' and e['id'] in raised:
            n += 12
        if v == 'v' and in16 and e['role'] in (1, 3):
            n -= 12
        if v == 'r' and in16 and e['role'] == 3:
            n -= 12
        if n != m:
            e['p'] = f'{440 * 2 ** ((n - 69) / 12):.4f}'
            changed.append((e['id'], e['role'], round(rel(e), 3), m, n))
        out.append(e)
    return out, changed


def sounding(ev, lo, hi):
    """(Vettore notes above the Corpo upper voice sounding at their onset, total, conflicts of class 1/6)"""
    corpo = [(e['f'], e['off'] or e['f'] + e['d'], E.midi(e)) for e in ev if e['role'] == 1]
    above = tot = conf = 0
    for e in ev:
        if e['role'] == 3 and lo - EPS <= rel(e) < hi - EPS:
            up = [m for s, x, m in corpo if s <= e['f'] + 24 < x]
            tot += 1
            above += bool(up) and E.midi(e) > max(up)
            conf += any((E.midi(e) - m) % 12 in (1, 11, 6) for m in up)
    return above, tot, conf


def leaps(ev):
    v = vett(ev)
    k0 = next(i for i, e in enumerate(v) if rel(e) >= -P15 - EPS) - 1  # last note of phrase 14
    v = [e for e in v[k0:] if rel(e) < 8 - EPS]
    m = [(rel(e), E.midi(e)) for e in v]
    d = [(b[0], abs(b[1] - a[1])) for a, b in zip(m, m[1:])]
    return {'max_into_A_prime': max(x for t, x in d if t < EPS), 'boundary': next(x for t, x in d if abs(t) < EPS)}


def write(a):
    ev = T.read(a.plan)
    d = Path(a.dir)
    d.mkdir(parents=True, exist_ok=True)
    for v in 'puvr':
        out, _ = variant(ev, v)
        out.sort(key=lambda e: (e['f'], e['id']))
        with open(d / f'{v}.csv', 'w', newline='\n') as w:
            for e in out:
                w.write(f"{e['id']},{e['role']},{e['f']},{e['d']},{e['p']},{e['v']},{e['t']},{e['off'] or ''}\n")
    print('switch', switch_point(ev)[:2])


def measure(a):
    ev = T.read(a.plan)
    d = Path(a.dir)
    man = json.loads((d / 'warped_plan_render_v1_manifest.json').read_text())
    out = {'manifest': man, 'switch_index_and_max_leap': switch_point(ev)[:2], 'versions': {}}
    ref = T.wav(a.reference)
    clips = {}
    for v in 'puvr':
        o, ch = variant(ev, v)
        x = T.wav(d / f'{v}.wav')
        if v == 'p':
            n = min(len(ref), len(x), int(182.85 * FS))
            out['X1_max_abs_difference'] = float(np.abs(ref[:n] - x[:n]).max())
        clips[v] = x[int((B0 - 16) * FS):int((B0 - 16) * FS) + 32 * FS]
        out['versions'][v] = {'changed': [list(c) for c in ch], 'leaps': leaps(o), 'phrase15_above_total_conflicts': sounding(o, -P15, 0),
                              'phrase16_above_total_conflicts': sounding(o, 0, 8), 'lufs': S.integrated_lufs(clips[v], FS)}
    V = out['versions']
    p = V['p']
    out['outcome'] = {
        'X1': out['X1_max_abs_difference'] <= 1e-3,
        'X2': not p['changed'] and all((c[4] - c[3]) % 12 == 0 for v in 'uvr' for c in V[v]['changed']) and all(V[v]['changed'] for v in 'uvr'),
        'X3': V['u']['leaps']['max_into_A_prime'] <= 9 and V['v']['leaps']['boundary'] <= 9,
        'X4': V['u']['phrase15_above_total_conflicts'][0] * 2 >= V['u']['phrase15_above_total_conflicts'][1] and
        V['v']['phrase16_above_total_conflicts'][0] * 2 >= V['v']['phrase16_above_total_conflicts'][1],
        'X5': all(V[v][k][2] == p[k][2] for v in 'uvr' for k in ('phrase15_above_total_conflicts', 'phrase16_above_total_conflicts')),
        'X6': man['status'] == 'MEASURED' and all(r['max_gain_reduction_db'] == 0 and r['staged_at_end'] == 0 for r in man['renders']) and
        all(abs(V[v]['lufs'] - p['lufs']) <= 0.5 for v in 'uvr')}
    if a.clips and out['outcome']['X1'] and out['outcome']['X2'] and out['outcome']['X6']:
        seed = secrets.randbits(32)
        names = list('puvr')
        random.Random(seed).shuffle(names)
        c = Path(a.clips)
        c.mkdir(parents=True, exist_ok=True)
        low, nf = min(V[v]['lufs'] for v in 'puvr'), FS // 100
        fade = np.linspace(0, 1, nf)[:, None]
        for i, k in enumerate(names, 1):
            y = clips[k] * 10 ** ((low - V[k]['lufs']) / 20)
            y[:nf] *= fade
            y[-nf:] *= fade[::-1]
            wavfile.write(c / f'ingresso_{i}.wav', FS, (y * 32767).astype(np.int16))
        Path(a.key).write_text('CHIAVE - non aprire prima di aver risposto\nseed ' + str(seed) + '\n' +
                               '\n'.join(f'ingresso_{i} = {k}' for i, k in enumerate(names, 1)) + '\n', encoding='utf-8')
    print(json.dumps(out, indent=1, default=lambda v: v.item()))


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=('write', 'measure'))
    for o in ('plan', 'dir', 'reference', 'clips', 'key'):
        ap.add_argument('--' + o)
    a = ap.parse_args()
    (write if a.mode == 'write' else measure)(a)
