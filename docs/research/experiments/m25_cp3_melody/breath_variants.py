"""A->B leap and breaths (docs/research/M2_5_CP3_I4_I6b_Breath_Preregistration.md). Same usage as two_bar_prototype.py."""
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
import two_bar_prototype as P  # noqa: E402

FS, T, S = P.FS, P.T, P.S
NAMES = 'bcde'
BOUNDARY = 64.0


class Build(P.Build):
    def __init__(self, ev, v):
        super().__init__(ev)
        self.v = v

    def prime(self, bars, src, b, kind):
        super().prime(bars, src, b, kind)
        if kind != 'answer' or self.v == 'b':
            return
        sc, q = P.sec(b * 16), bars[b]
        e, prev = q[-1], P.midi(q[-2])
        old = P.midi(next(x for x in src[b] if P.slot(x['f']) % 16 >= 8))
        nxt = P.midi(src[b + 1][0]) if b + 1 in src else None
        top = max(P.corpo_at(self.ev, e['f']), default=0)
        cand = [x for x in range(24, 108) if x % 12 == P.TONIC[sc] and not P.clash(self.ev, e['f'], x)]

        def cost(x):
            leap = max(abs(prev - x), abs(nxt - x) if nxt is not None else 0)
            return x <= top, leap, abs(x - old)
        e['p'] = P.hz(min(cand, key=cost))
        if self.v == 'd' and (b // 4) % 8 not in (3, 7):
            e['d'] = P.frame(b * 16 + 16) - e['f']

    def rhythm(self, role):
        out = super().rhythm(role)
        if role == 0 and self.v == 'e':
            for b in range(96):
                q = [e for e in out if P.slot(e['f']) // 16 == b]
                if b % 8 == 7 and q and b * 16 + 15 not in {P.slot(e['f']) for e in q}:
                    last = max((e for e in q if P.slot(e['f']) % 16 < 14), key=lambda e: e['f'])
                    n = dict(last)
                    self.new += 1
                    n.update(id=self.new, f=P.frame(b * 16 + 15), v=f"{float(last['v']) * 0.85:.4f}")
                    out.append(n)
        return out


def variant(ev, v):
    k = Build(ev, v)
    out = [dict(e) for e in ev if e['role'] == 1] + k.melody() + k.rhythm(0) + k.rhythm(2)
    return sorted(out, key=lambda e: (e['f'], e['id']))


def write(a):
    ev = T.read(a.plan)
    d = Path(a.dir)
    d.mkdir(parents=True, exist_ok=True)
    for v in NAMES:
        with open(d / f'{v}.csv', 'w', newline='\n') as w:
            for e in variant(ev, v):
                w.write(f"{e['id']},{e['role']},{e['f']},{e['d']},{e['p']},{e['v']},{e['t']},{e['off'] or ''}\n")


def leaps(o):
    q = sorted((e for e in o if e['role'] == 3), key=lambda e: e['f'])
    d = [(P.slot(y['f']), P.midi(y) - P.midi(x)) for x, y in zip(q, q[1:])]
    return {'boundary_AB': next(m for s, m in d if s == 512), 'over_12': [(s // 16, m) for s, m in d if abs(m) > 12]}


def breaths(o):
    n = 0
    for p in range(24):
        bars = [[e for e in o if e['role'] == 3 and P.slot(e['f']) // 16 == p * 4 + k] for k in range(4)]
        last = max(bars[3], key=lambda e: e['f'])
        n += last['f'] + last['d'] <= P.frame((p * 4 + 4) * 16) - P.frame(64) // 32
    return n


def diff(x, y):
    def key(e):
        return e['role'], e['f'], e['d'], e['p'], e['v'], e['t'], e['off']
    kx, ky = [key(e) for e in x], [key(e) for e in y]
    return [k for k in kx if k not in set(ky)], [k for k in ky if k not in set(kx)]


def measure(a):
    ev = T.read(a.plan)
    d = Path(a.dir)
    man = json.loads((d / 'warped_plan_render_v1_manifest.json').read_text())
    out, V, clips, O = {'manifest': man}, {}, {}, {v: variant(ev, v) for v in NAMES}
    n = int((32 + 32 * 60 / 140) * FS)
    for v in NAMES:
        x = T.wav(d / f'{v}.wav')
        clips[v] = x[32 * FS:32 * FS + n].copy()
        V[v] = P.symbolic(O[v])
        V[v].update(leaps=leaps(O[v]), breath_bars=breaths(O[v]), lufs=S.integrated_lufs(clips[v], FS), lufs_whole=S.integrated_lufs(x, FS))
    rest = P.variant(ev, 'b')[0]
    bc, cb = diff(O['b'], O['c'])
    cd, dc = diff(O['c'], O['d'])
    ce, ec = diff(O['c'], O['e'])

    def ans(k):
        return (P.slot(k[1]) // 16) % 8 == 7
    out['versions'], b = V, V['b']
    out['outcome'] = {
        'W1': diff(rest, O['b']) == ([], []) and all(k[0] == 3 and ans(k) for k in bc + cb) and
        all(k[0] == 3 and ans(k) for k in cd + dc) and [k[:2] + k[3:] for k in cd] == [k[:2] + k[3:] for k in dc] and
        not ce and all(k[0] == 0 and ans(k) for k in ec) and bool(cb) and bool(dc) and bool(ec),
        'W2': all(abs(V[v]['leaps']['boundary_AB']) <= 12 and {x[0] for x in V[v]['leaps']['over_12']} <= {x[0] for x in b['leaps']['over_12']} for v in 'cde'),
        'W3': all(V[v]['conflicts'] == 0 and all(not (x >= 0.5 and y < 0.5) for x, y in zip(b['above_share_per_phrase'], V[v]['above_share_per_phrase'])) for v in 'cde'),
        'W4': all(V[v]['end_on_tonic']['odd'] == 1 for v in 'cde') and all(V[v]['breath_bars'] == 12 for v in 'bce') and V['d']['breath_bars'] == 6,
        'W5': all(V[v]['M1'] < 0.6 and V[v]['G1']['3'] == b['G1']['3'] for v in 'cde'),
        'W6': man['status'] == 'MEASURED' and len(man['renders']) == 4 and
        all(r['max_gain_reduction_db'] == 0 and r['staged_at_end'] == 0 and r['backpressure_drops'] == 0 for r in man['renders']) and
        all(abs(V[v]['lufs'] - b['lufs']) <= 0.5 and abs(V[v]['lufs_whole'] - b['lufs_whole']) <= 0.5 for v in 'cde')}
    if a.clips and out['outcome']['W1'] and out['outcome']['W6']:
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
            wavfile.write(c / f'respiro_{i}.wav', FS, (y * 32767).astype(np.int16))
        Path(a.key).write_text('CHIAVE - non aprire prima di aver risposto\nseed ' + str(seed) + '\n' +
                               '\n'.join(f'respiro_{i} = {k}' for i, k in enumerate(names, 1)) + '\n', encoding='utf-8')
    out['changes'] = {'bc': cb, 'cd': len(dc), 'ce': len(ec)}
    Path(a.out).write_text(json.dumps(out, indent=1, default=lambda v: v.item()) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(out['outcome']))
    print('changed', len(cb), len(dc), len(ec), [(P.slot(k[1]) // 16, k[3]) for k in cb], [P.slot(k[1]) // 16 for k in ec])
    for v in NAMES:
        print(v, V[v]['leaps'], V[v]['breath_bars'], V[v]['M1'], V[v]['end_on_tonic'], V[v]['conflicts'], round(V[v]['lufs'], 2))


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=('write', 'measure'))
    for o in ('plan', 'dir', 'reference', 'clips', 'key', 'out'):
        ap.add_argument('--' + o)
    a = ap.parse_args()
    (write if a.mode == 'write' else measure)(a)
