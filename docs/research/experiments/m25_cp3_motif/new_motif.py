"""Three authored motifs for section A (docs/research/M2_5_CP3_I9_New_Motif_Preregistration.md).

  python -I new_motif.py write   --plan <csv> --dir artifacts/cacp/tempo_ramp_v1   (empty directory)
  cymatica_game --cacp-run warped_plan_render_v1
  python -I new_motif.py measure --plan <csv> --dir ... --out <json> [--clips <dir> --key <file>]
"""
import argparse
import json
import random
import secrets
import sys
from pathlib import Path

import numpy as np
from scipy.io import wavfile

HERE = Path(__file__).resolve().parent
for q in (HERE.parent / 'm25_cp3_accents', HERE.parent / 'm25_cp3_melody', HERE.parent / 'expert_production_analysis'):
    sys.path.insert(0, str(q))
import accent_prototype as A  # noqa: E402
import two_bar_prototype as P  # noqa: E402

T, S, CM3, BP, FS = A.T, A.S, A.CM3, A.BP, 48000
GAIN_DB = (1.9778700563168066, 1.3465195633607498, 1.4909743671615452)  # bass level -3 LU (bass_level.json control)
MOTIFS = {
    'N1': [(0, 8, 0), (8, 4, 0), (12, 4, 2), (16, 8, 4), (24, 4, 3), (28, 4, 2), (32, 8, 4), (40, 4, 4), (44, 4, 6), (48, 8, 4), (56, 8, 0)],
    'N2': [(0, 8, 0), (8, 4, 2), (12, 4, 3), (16, 8, 5), (24, 8, 3), (32, 12, 6), (44, 4, 4), (48, 6, 2), (54, 2, 1), (56, 8, 0)],
    'N3': [(0, 3, 4), (3, 3, 2), (6, 6, 0), (20, 3, 3), (23, 3, 5), (26, 6, 7), (32, 3, 6), (35, 3, 4), (38, 6, 1), (52, 4, 2), (56, 8, 0)]}
FORM = ['S', 'SP', 'V', 'SP', 'S', 'F', 'V', 'C']
DOR = [0, 2, 3, 5, 7, 9, 10]
NAMES = ('p', 'N1', 'N2', 'N3')


def deg(d):
    return 74 + 12 * (d // 7) + DOR[d % 7]


def phrase(m, kind):
    q = list(m)
    if kind == 'S':
        q[-1] = (q[-1][0], q[-1][1], 4)
    if kind == 'V':
        q = [(s, d, g + 2 if 32 <= s < 48 else g) for s, d, g in q]
    if kind == 'F':
        q = [x for x in q if x[0] < 32] + [(s + 32, d, g) for s, d, g in q if s < 16]
    return q


def melody(ev, name):
    old = [e for e in ev if e['role'] == 3 and P.slot(e['f']) < 512]
    out, moved, nid = [], 0, 4 * 10 ** 6
    for p, kind in enumerate(FORM):
        tmpl = min((e for e in old if P.slot(e['f']) // 64 == p), key=lambda e: e['f'])
        for s, d, g in phrase(MOTIFS[name], kind):
            f = P.frame(p * 64 + s)
            m = deg(g)
            if kind == 'C' and s < 16 and m - 12 > max(P.corpo_at(ev, f), default=0):
                m -= 12
                g -= 7
            if P.clash(ev, f, m):
                for k in (-1, 1, -2, 2):
                    if not P.clash(ev, f, deg(g + k)):
                        m = deg(g + k)
                        moved += 1
                        break
            e = dict(tmpl)
            nid += 1
            e.update(id=nid, f=f, d=P.frame(p * 64 + s + d) - f, off=0, p=P.hz(m))
            out.append(e)
    return out, moved


def plan(ev, name):
    if name == 'p':
        return [dict(e) for e in ev], 0
    new, moved = melody(ev, name)
    return [dict(e) for e in ev if not (e['role'] == 3 and P.slot(e['f']) < 512)] + new, moved


def write(a):
    ev = T.read(a.plan)
    d = Path(a.dir)
    d.mkdir(parents=True, exist_ok=True)
    for n in NAMES:
        x, moved = plan(ev, n)
        CM3.rows(d, f'k3_{n}', x)
        print(n, 'moved', moved)
    CM3.rows(d, 'bass_line', A.bass_rows(A.bass_line(ev, 'g0')))


def sym(ev, o):
    q = sorted((e for e in o if e['role'] == 3 and P.slot(e['f']) < 512), key=lambda e: e['f'])
    bars = [[] for _ in range(32)]
    ph = [[] for _ in range(8)]
    for e in q:
        bars[P.slot(e['f']) // 16].append((P.slot(e['f']) % 16, P.midi(e)))
        c = P.corpo_at(ev, e['f'])
        ph[P.slot(e['f']) // 64].append(bool(c) and P.midi(e) > max(c))
    con = [tuple(np.sign(np.diff([m for _, m in b])).astype(int)) for b in bars]
    same = [con[p * 4 + k] == con[p * 4] for p in range(8) for k in (1, 2, 3)]
    gap = 0
    for x, y in zip(q, q[1:]):
        gap = max(gap, (y['f'] - x['f'] - x['d']) / 6000)
    rest_f = [(P.slot(e['f']) + round(e['d'] / 6000)) for e in q if 320 <= P.slot(e['f']) < 384]
    return {'notes': len(q), 'conflicts': sum(P.clash(ev, e['f'], P.midi(e)) for e in q),
            'above_min_share': min(sum(x) / len(x) for x in ph), 'max_leap': max(abs(P.midi(b) - P.midi(a)) for a, b in zip(q, q[1:])),
            'same_contour': float(np.mean(same)), 'phrases_on_tonic': sum(b[-1][1] % 12 == 2 for b in (next(x for x in reversed(bars[p * 4:p * 4 + 4]) if x) for p in range(8))),
            'longest_rest_sixteenths': float(gap), 'last_end_phrase_F': max(rest_f), 'onsets': [sorted(x[0] for x in b) for b in bars],
            'range': [min(P.midi(e) for e in q), max(P.midi(e) for e in q)]}


def jac(x, y):
    v = []
    for a, b in zip(x['onsets'], y['onsets']):
        u = set(a) | set(b)
        v.append(len(set(a) & set(b)) / len(u) if u else 1.0)
    return float(np.mean(v))


def measure(a):
    ev, d = T.read(a.plan), Path(a.dir)
    man = json.loads((d / 'warped_plan_render_v1_manifest.json').read_text())
    n = min(len(T.wav(d / f'k3_{v}.wav')) for v in NAMES)
    edges = [int(x * FS) for x in (*BP.START, BP.START[2] + 64.0)]
    g = np.full(n, 10 ** (GAIN_DB[2] / 20))
    for s in range(3):
        g[edges[s]:edges[s + 1]] = 10 ** (GAIN_DB[s] / 20)
    bass = np.column_stack([CM3.mono(T.wav(d / 'bass_line.wav'), n)] * 2) * g[:, None]
    V, clips, lu, o1 = {}, {}, {}, True

    def key(x):
        return sorted((e['role'], e['f'], e['d'], e['p'], e['v'], e['t'], e['off']) for e in x if not (e['role'] == 3 and P.slot(e['f']) < 512))
    for v in NAMES:
        o, moved = plan(ev, v)
        o1 &= key(o) == key(ev)
        mix = T.wav(d / f'k3_{v}.wav')[:n] + bass
        V[v] = sym(ev, o)
        V[v].update(moved=moved, peak_dbfs=float(20 * np.log10(np.abs(mix).max())))
        clips[v] = mix[:64 * FS].copy()
        lu[v] = S.integrated_lufs(clips[v], FS)
    nf, fin = FS // 100, {}
    for v in NAMES:
        y = clips[v] * 10 ** ((min(lu.values()) - lu[v]) / 20)
        y[:nf] *= np.linspace(0, 1, nf)[:, None]
        y[-nf:] *= np.linspace(1, 0, nf)[:, None]
        fin[v] = (y * 32767).astype(np.int16)
    lf = {v: S.integrated_lufs(fin[v] / 32768.0, FS) for v in NAMES}
    J = {f'{x}-{y}': jac(V[x], V[y]) for i, x in enumerate(NAMES) for y in NAMES[i + 1:]}
    N = NAMES[1:]
    out = {'versions': V, 'jaccard': J, 'lufs_before': lu, 'manifest': man, 'outcome': {
        'O1': o1, 'O2': max(J.values()) <= 0.6, 'O3': all(V[v]['conflicts'] == 0 and V[v]['moved'] <= 0.2 * V[v]['notes'] for v in N),
        'O4': all(V[v]['above_min_share'] >= 0.5 for v in N), 'O5': all(V[v]['max_leap'] <= 12 for v in N),
        'O6': all(V[v]['same_contour'] < 0.6 for v in N), 'O7': all(V[v]['phrases_on_tonic'] >= 4 for v in N) and V['N3']['longest_rest_sixteenths'] >= 8 and all(V[v]['last_end_phrase_F'] <= 320 + 48 + 8 for v in N),
        'O8': man['status'] == 'MEASURED' and len(man['renders']) == 5 and all(r['max_gain_reduction_db'] == 0 and r['staged_at_end'] == 0 and r['backpressure_drops'] == 0 for r in man['renders']) and
        all(V[v]['peak_dbfs'] <= -1 for v in NAMES) and max(lf.values()) - min(lf.values()) <= 0.1}}
    if a.clips and out['outcome']['O1'] and out['outcome']['O8']:
        seed = secrets.randbits(32)
        names = list(NAMES)
        random.Random(seed).shuffle(names)
        c = Path(a.clips)
        c.mkdir(parents=True, exist_ok=True)
        for i, k in enumerate(names, 1):
            wavfile.write(c / f'motivo_{i}.wav', FS, fin[k])
        Path(a.key).write_text(f'CHIAVE - non aprire prima di aver risposto\nseed {seed}\n' + '\n'.join(f'motivo_{i} = {k}' for i, k in enumerate(names, 1)) + '\n', encoding='utf-8')
    Path(a.out).write_text(json.dumps(out, indent=1, default=lambda v: v.item()) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(out['outcome']), {k: round(x, 2) for k, x in J.items()})
    for v in NAMES:
        print(v, {k: x for k, x in V[v].items() if k != 'onsets'})


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=('write', 'measure'))
    for o in ('plan', 'dir', 'out', 'clips', 'key'):
        ap.add_argument('--' + o)
    a = ap.parse_args()
    (write if a.mode == 'write' else measure)(a)
