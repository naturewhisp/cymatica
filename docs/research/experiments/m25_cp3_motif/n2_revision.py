"""I9 ter, revision of N2 (docs/research/M2_5_CP3_I9c_N2_Revision_Preregistration.md). Usage as whole_piece.py; --ref <k3_B2.wav of I9 bis>, --refplan <k3_B2.csv of I9 bis>."""
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
import whole_piece as W  # noqa: E402

N, P, T, S, A, CM3, BP, FS = W.N, W.P, W.T, W.S, W.A, W.CM3, W.BP, 48000
NAMES = ('Y', 'R1', 'R2', 'R12')
CLOSE = [4, 2, 4, 0, 4, None, 2, 0]
FORMS = (N.FORM, N.FORM, W.FORM_AP)
TONIC = (2, 7, 2)


def phrase(sec, p, name):
    kind = FORMS[sec][p]
    mirror = sec == 1
    q = list(W.phrase(kind, mirror))
    if '1' in name and kind == 'F':
        q += [(s, d, 7 - g if mirror else g) for s, d, g in ((48, 6, 4), (54, 2, 3), (56, 8, 2))]
    if '2' in name:
        if kind in ('SP', 'V', 'C', 'Z'):
            q[0] = (*q[0][:2], 4)
        if kind not in ('F', 'Z'):
            c = CLOSE[p]
            q[-1] = (*q[-1][:2], 7 if mirror and c == 0 else c)
    return q


def plan(ev, name):
    out = [dict(e) for e in ev if e['role'] != 3]
    moved = {}
    for sec in range(3):
        moved[sec] = 0
        if sec < 2:  # same event ids as I9 bis
            nid = (4 + sec) * 10 ** 6
        for p in range(8):
            ph = sec * 8 + p
            tmpl = min((e for e in ev if e['role'] == 3 and P.slot(e['f']) // 64 == ph), key=lambda e: e['f'])
            for s, d, g in phrase(sec, p, name):
                f = P.frame(ph * 64 + s)
                m = W.deg(sec, g)
                if P.clash(ev, f, m):
                    for k in (-1, 1, -2, 2):
                        if not P.clash(ev, f, W.deg(sec, g + k)):
                            m = W.deg(sec, g + k)
                            moved[sec] += 1
                            break
                nid += 1
                e = dict(tmpl)
                e.update(id=nid, f=f, d=P.frame(ph * 64 + s + d) - f, off=0, p=P.hz(m))
                out.append(e)
    return out, moved


def notes(x):
    return sorted((e['role'], e['f'], e['d'], e['p'], e['v'], e['t'], e['off']) for e in x)


def write(a):
    ev = T.read(a.plan)
    d = Path(a.dir)
    d.mkdir(parents=True, exist_ok=True)
    for n in NAMES:
        x, moved = plan(ev, n)
        CM3.rows(d, f'k3_{n}', x)
        print(n, moved)
    print('Y equals I9 bis B2 notes:', notes(plan(ev, 'Y')[0]) == notes(W.plan(ev, 'B2')[0]))
    CM3.rows(d, 'bass_line', A.bass_rows(A.bass_line(ev, 'g0')))


def sym(ev, o):
    r = W.sym(ev, o)
    q = sorted((e for e in o if e['role'] == 3), key=lambda e: e['f'])
    bars = {P.slot(e['f']) // 16 for e in q}
    r['empty_bars'] = [b for b in range(96) if b not in bars]
    op = [P.midi(min((e for e in q if P.slot(e['f']) // 64 == p), key=lambda e: e['f'])) % 12 == TONIC[p // 8] for p in range(24)]
    r['tonic_openings'] = [sum(op[s * 8:s * 8 + 8]) for s in range(3)]
    r['tonic_duration_share'] = []
    for s in range(3):
        x = [e for e in q if P.slot(e['f']) // 512 == s]
        r['tonic_duration_share'].append(sum(e['d'] for e in x if P.midi(e) % 12 == TONIC[s]) / sum(e['d'] for e in x))
    r['rhythm'] = [[P.slot(e['f']) % 64 for e in q if P.slot(e['f']) // 64 == p] for p in range(24)]
    return r


def measure(a):
    ev, d = T.read(a.plan), Path(a.dir)
    man = json.loads((d / 'warped_plan_render_v1_manifest.json').read_text())
    n = min(len(T.wav(d / f'k3_{v}.wav')) for v in NAMES)
    edges = [int(x * FS) for x in (*BP.START, BP.START[2] + 64.0)]
    g = np.full(n, 10 ** (N.GAIN_DB[2] / 20))
    for s in range(3):
        g[edges[s]:edges[s + 1]] = 10 ** (N.GAIN_DB[s] / 20)
    bass = np.column_stack([CM3.mono(T.wav(d / 'bass_line.wav'), n)] * 2) * g[:, None]
    ref = T.wav(a.ref)
    V, clips, lu, x1 = {}, {}, {}, True
    for v in NAMES:
        o, moved = plan(ev, v)
        x1 &= [x for x in notes(o) if x[0] != 3] == [x for x in notes(ev) if x[0] != 3]
        k = T.wav(d / f'k3_{v}.wav')[:n]
        if v == 'Y':
            m = min(len(k), len(ref))
            diff = float(np.abs(k[:m] - ref[:m]).max())
            length = len(k) == len(ref)
        mix = k + bass
        V[v] = sym(ev, o)
        V[v].update(moved=moved, peak_dbfs=float(20 * np.log10(np.abs(mix).max())))
        wavfile.write(d / f'piece_{v}.wav', FS, (np.clip(mix, -1, 1) * 32767).astype(np.int16))
        clips[v] = mix[56 * FS:134 * FS].copy()
        lu[v] = S.integrated_lufs(clips[v], FS)
    same = notes(plan(ev, 'Y')[0]) == notes(W.plan(ev, 'B2')[0])
    if a.refplan:
        same &= (d / 'k3_Y.csv').read_bytes() == Path(a.refplan).read_bytes()
    nf, fin = FS // 100, {}
    for v in NAMES:
        y = clips[v] * 10 ** ((min(lu.values()) - lu[v]) / 20)
        y[:nf] *= np.linspace(0, 1, nf)[:, None]
        y[-nf:] *= np.linspace(1, 0, nf)[:, None]
        fin[v] = (y * 32767).astype(np.int16)
    lf = {v: S.integrated_lufs(fin[v] / 32768.0, FS) for v in NAMES}
    R2 = ('R2', 'R12')
    tonic = {v: V[v]['tonic_duration_share'] for v in NAMES}
    kinds = [FORMS[p // 8][p % 8] for p in range(24)]
    out = {'versions': V, 'Y_render_difference': diff, 'Y_same_length': length, 'lufs_before': lu, 'lufs_clips': lf, 'manifest': man, 'outcome': {
        'X1': bool(x1 and same and diff <= 1e-3 and length),
        'X2': all(not V[v]['empty_bars'] for v in ('R1', 'R12')) and all(V[v]['empty_bars'] == [23, 55, 87] for v in ('Y', 'R2')),
        'X3': all(V[v]['conflicts'] == 0 and all(V[v]['moved'][s] <= 0.2 * V[v]['notes_per_section'][s] for s in range(3)) for v in NAMES),
        'X4': all(V[v]['above_min_share'] >= 0.5 for v in NAMES),
        'X5': all(V[v]['max_leap'] <= 12 and V[v]['boundary_AB'] <= 9 and V[v]['boundary_BA'] <= 9 for v in NAMES),
        'X6': all(max(V[v]['tonic_openings']) <= 4 and max(V[v]['tonic_per_section']) <= 3 and V[v]['last_on_tonic'] for v in R2),
        'X7': all(tonic[v][s] <= tonic['Y'][s] * 2 / 3 for v in R2 for s in range(3)),
        'X8': all(V[v]['rhythm'][p] == V['Y']['rhythm'][0] for v in NAMES for p in range(24) if kinds[p] in ('S', 'SP', 'V', 'C')),
        'X9': man['status'] == 'MEASURED' and len(man['renders']) == 5 and all(r['max_gain_reduction_db'] == 0 and r['staged_at_end'] == 0 and r['backpressure_drops'] == 0 for r in man['renders']) and
        all(V[v]['peak_dbfs'] <= -1 for v in NAMES) and max(lf.values()) - min(lf.values()) <= 0.1}}
    if a.clips and out['outcome']['X1'] and out['outcome']['X9']:
        seed = secrets.randbits(32)
        rng = random.Random(seed)
        names = list(NAMES) + [rng.choice(NAMES)]
        rng.shuffle(names)
        c = Path(a.clips)
        c.mkdir(parents=True)
        for i, k in enumerate(names, 1):
            wavfile.write(c / f'estratto_{i}.wav', FS, fin[k])
        Path(a.key).write_text(f'CHIAVE - non aprire prima di aver risposto\nseed {seed}\n' + '\n'.join(f'estratto_{i} = {k}' for i, k in enumerate(names, 1)) + '\n', encoding='utf-8')
    Path(a.out).write_text(json.dumps(out, indent=1, default=lambda v: v.item()) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(out['outcome']), diff)
    for v in NAMES:
        print(v, {k: x for k, x in V[v].items() if k != 'rhythm'})


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=('write', 'measure'))
    for o in ('plan', 'dir', 'out', 'clips', 'key', 'ref', 'refplan'):
        ap.add_argument('--' + o)
    a = ap.parse_args()
    (write if a.mode == 'write' else measure)(a)
