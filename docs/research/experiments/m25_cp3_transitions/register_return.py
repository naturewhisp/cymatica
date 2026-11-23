"""Low entry of A' with a joined register return (docs/research/M2_5_CP3_I5d_Register_Return_Preregistration.md).
Same usage as melody_approach.py."""
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
import melody_approach as M  # noqa: E402
import stem_analysis as S  # noqa: E402
import tempo_ramp as T  # noqa: E402

E, FS, B0, EPS, rel = M.E, M.FS, M.B0, M.EPS, M.rel
NAMES = 'pvwx'


def switch(ev):
    """(time, max leap) of the first Vettore note of phrase 16 back at today's register."""
    n16 = [e for e in M.vett(ev) if -EPS <= rel(e) < 8 - EPS]
    m = [E.midi(e) for e in n16]
    end = E.midi(next(e for e in M.vett(ev) if rel(e) >= 8 - EPS))
    best = None
    for k in range(1, len(m)):
        q = [x - (12 if i < k else 0) for i, x in enumerate(m)] + [end]
        top = max(abs(b - a) for a, b in zip(q, q[1:]))
        if best is None or top <= best[1]:
            best = (rel(n16[k]), top)
    return best


def variant(ev, v):
    end = {3: {'p': 0, 'v': 8, 'w': switch(ev)[0], 'x': 4}[v], 1: {'p': 0, 'v': 8, 'w': 8, 'x': 4}[v]}
    out, changed = [], []
    for e in ev:
        e = dict(e)
        if e['role'] in end and -EPS <= rel(e) < end[e['role']] - EPS:
            n = E.midi(e)
            e['p'] = f'{440 * 2 ** ((n - 81) / 12):.4f}'
            changed.append((e['id'], e['role'], round(rel(e), 3), n, n - 12))
        out.append(e)
    return out, changed


def leaps(ev):
    n = M.vett(ev)
    d = [(rel(b), abs(E.midi(b) - E.midi(a))) for a, b in zip(n, n[1:])]
    return {'boundary': next(x for t, x in d if abs(t) < EPS), 'inside': max(x for t, x in d if EPS < t < 8 + EPS),
            'into_phrase_17': next(x for t, x in d if abs(t - 8) < EPS)}


def write(a):
    ev = T.read(a.plan)
    d = Path(a.dir)
    d.mkdir(parents=True, exist_ok=True)
    for v in NAMES:
        out = sorted(variant(ev, v)[0], key=lambda e: (e['f'], e['id']))
        with open(d / f'{v}.csv', 'w', newline='\n') as w:
            for e in out:
                w.write(f"{e['id']},{e['role']},{e['f']},{e['d']},{e['p']},{e['v']},{e['t']},{e['off'] or ''}\n")
    print('switch', switch(ev))


def measure(a):
    ev = T.read(a.plan)
    d = Path(a.dir)
    man = json.loads((d / 'warped_plan_render_v1_manifest.json').read_text())
    out, V, clips, ref = {'manifest': man, 'switch_w': switch(ev)}, {}, {}, T.wav(a.reference)
    for v in NAMES:
        o, ch = variant(ev, v)
        x = T.wav(d / f'{v}.wav')
        if v == 'p':
            n = min(len(ref), len(x), int(182.85 * FS))
            out['Y1_max_abs_difference'] = float(np.abs(ref[:n] - x[:n]).max())
        clips[v] = x[int((B0 - 16) * FS):int((B0 - 16) * FS) + 32 * FS]
        V[v] = {'changed': ch, 'leaps': leaps(o), 'above_total_conflicts': M.sounding(o, 0, 8), 'lufs': S.integrated_lufs(clips[v], FS)}
    out['versions'], p = V, V['p']
    out['outcome'] = {
        'Y1': out['Y1_max_abs_difference'] <= 1e-3,
        'Y2': not p['changed'] and all(V[v]['changed'] and all(c[3] - c[4] == 12 and c[1] in (1, 3) and -EPS <= c[2] < 8 for c in V[v]['changed']) for v in 'wx'),
        'Y3': all(V[v]['leaps']['boundary'] <= 9 for v in 'vwx') and all(V[v]['leaps']['inside'] <= 10 for v in 'wx'),
        'Y4': all(V[v]['above_total_conflicts'][0] * 2 >= V[v]['above_total_conflicts'][1] for v in 'vwx'),
        'Y5': all(V[v]['above_total_conflicts'][2] == p['above_total_conflicts'][2] for v in 'vwx'),
        'Y6': man['status'] == 'MEASURED' and len(man['renders']) == 4 and
        all(r['max_gain_reduction_db'] == 0 and r['staged_at_end'] == 0 for r in man['renders']) and
        all(abs(V[v]['lufs'] - p['lufs']) <= 0.5 for v in 'vwx')}
    if a.clips and all(out['outcome'][k] for k in ('Y1', 'Y2', 'Y6')):
        seed = secrets.randbits(32)
        names = list(NAMES)
        random.Random(seed).shuffle(names)
        c = Path(a.clips)
        c.mkdir(parents=True, exist_ok=True)
        low, nf = min(V[v]['lufs'] for v in NAMES), FS // 100
        fade = np.linspace(0, 1, nf)[:, None]
        for i, k in enumerate(names, 1):
            y = clips[k] * 10 ** ((low - V[k]['lufs']) / 20)
            y[:nf] *= fade
            y[-nf:] *= fade[::-1]
            wavfile.write(c / f'raccordo_{i}.wav', FS, (y * 32767).astype(np.int16))
        Path(a.key).write_text('CHIAVE - non aprire prima di aver risposto\nseed ' + str(seed) + '\n' +
                               '\n'.join(f'raccordo_{i} = {k}' for i, k in enumerate(names, 1)) + '\n', encoding='utf-8')
    Path(a.out).write_text(json.dumps(out, indent=1, default=lambda v: v.item()) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({k: out[k] for k in ('outcome', 'switch_w', 'Y1_max_abs_difference')}, default=str))
    for v in NAMES:
        print(v, len(V[v]['changed']), V[v]['leaps'], V[v]['above_total_conflicts'], round(V[v]['lufs'], 2))


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=('write', 'measure'))
    for o in ('plan', 'dir', 'reference', 'clips', 'key', 'out'):
        ap.add_argument('--' + o)
    a = ap.parse_args()
    (write if a.mode == 'write' else measure)(a)
