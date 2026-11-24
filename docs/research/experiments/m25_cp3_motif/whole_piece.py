"""N2 on the whole piece (docs/research/M2_5_CP3_I9b_Whole_Piece_Preregistration.md). Same usage as new_motif.py, plus --ref <k3_N2.wav of I9>."""
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
import new_motif as N  # noqa: E402

P, T, S, A, CM3, BP, FS = N.P, N.T, N.S, N.A, N.CM3, N.BP, 48000
NAMES = ('B0', 'B1', 'B2')
AEO = [0, 2, 3, 5, 7, 8, 10]
FORM_AP = ['S', 'SP', 'V', 'SP', 'S', 'F', 'V', 'Z']


def deg(sec, g):
    base, sc = (67, AEO) if sec == 1 else (74, N.DOR)
    return base + 12 * (g // 7) + sc[g % 7]


def phrase(kind, mirror):
    q = list(N.MOTIFS['N2'])
    if mirror:
        q = [(s, d, 7 - g) for s, d, g in q]
    if kind == 'S':
        q[-1] = (*q[-1][:2], 4)
    if kind == 'V':
        q = [(s, d, g + (-2 if mirror else 2) if 32 <= s < 48 else g) for s, d, g in q]
    if kind == 'F':
        q = [x for x in q if x[0] < 32] + [(s + 32, d, g) for s, d, g in q if s < 16]
    if kind == 'Z':
        q = [x for x in q if x[0] < 48] + [(48, 16, 0)]
    return q


def plan(ev, name):
    out = [dict(e) for e in N.plan(ev, 'N2')[0] if e['role'] != 3 or P.slot(e['f']) < 512]
    today = [e for e in ev if e['role'] == 3 and 512 <= P.slot(e['f']) < 1024]
    moved, nid = {0: N.plan(ev, 'N2')[1], 1: 0, 2: 0}, 5 * 10 ** 6
    for sec, form in ((1, N.FORM), (2, FORM_AP)):
        if sec == 1 and name == 'B0':
            out += [dict(e) for e in today]
            continue
        for p, kind in enumerate(form):
            ph = sec * 8 + p
            tmpl = min((e for e in ev if e['role'] == 3 and P.slot(e['f']) // 64 == ph), key=lambda e: e['f'])
            for s, d, g in phrase(kind, sec == 1 and name == 'B2'):
                f = P.frame(ph * 64 + s)
                m = deg(sec, g)
                if P.clash(ev, f, m):
                    for k in (-1, 1, -2, 2):
                        if not P.clash(ev, f, deg(sec, g + k)):
                            m = deg(sec, g + k)
                            moved[sec] += 1
                            break
                e = dict(tmpl)
                nid += 1
                e.update(id=nid, f=f, d=P.frame(ph * 64 + s + d) - f, off=0, p=P.hz(m))
                out.append(e)
    return out, moved


def write(a):
    ev = T.read(a.plan)
    d = Path(a.dir)
    d.mkdir(parents=True, exist_ok=True)
    for n in NAMES:
        x, moved = plan(ev, n)
        CM3.rows(d, f'k3_{n}', x)
        print(n, moved)
    CM3.rows(d, 'bass_line', A.bass_rows(A.bass_line(ev, 'g0')))


def sym(ev, o):
    q = sorted((e for e in o if e['role'] == 3), key=lambda e: e['f'])
    r = {}
    d = [(P.slot(y['f']), abs(P.midi(y) - P.midi(x))) for x, y in zip(q, q[1:])]
    r['boundary_AB'] = next(x for s, x in d if s == 512)
    r['boundary_BA'] = next(x for s, x in d if s == 1024)
    r['max_leap'] = max(x for _, x in d)
    r['conflicts'] = sum(P.clash(ev, e['f'], P.midi(e)) for e in q)
    ph = [[] for _ in range(24)]
    for e in q:
        c = P.corpo_at(ev, e['f'])
        ph[P.slot(e['f']) // 64].append(bool(c) and P.midi(e) > max(c))
    r['above_min_share'] = min(sum(x) / len(x) for x in ph)
    ton = []
    for p in range(24):
        last = max((e for e in q if P.slot(e['f']) // 64 == p), key=lambda e: e['f'])
        ton.append(P.midi(last) % 12 == (7 if p // 8 == 1 else 2))
    r['tonic_per_section'] = [sum(ton[s * 8:s * 8 + 8]) for s in range(3)]
    r['last_on_tonic'] = bool(ton[-1])
    bars = [[P.midi(e) for e in q if P.slot(e['f']) // 16 == b] for b in range(96)]
    con = [tuple(np.sign(np.diff(b)).astype(int)) for b in bars]
    r['same_contour_B'] = float(np.mean([con[p * 4 + k] == con[p * 4] for p in range(8, 16) for k in (1, 2, 3)]))
    r['notes_per_section'] = [sum(P.slot(e['f']) // 512 == s for e in q) for s in range(3)]
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
    V, clips, lu, u1, refdiff = {}, {}, {}, True, 0.0

    def key(x):
        return sorted((e['role'], e['f'], e['d'], e['p'], e['v'], e['t'], e['off']) for e in x if e['role'] != 3)
    for v in NAMES:
        o, moved = plan(ev, v)
        u1 &= key(o) == key(ev)
        k = T.wav(d / f'k3_{v}.wav')[:n]
        refdiff = max(refdiff, float(np.abs(k[:63 * FS] - ref[:63 * FS]).max()))
        mix = k + bass
        V[v] = sym(ev, o)
        V[v].update(moved=moved, peak_dbfs=float(20 * np.log10(np.abs(mix).max())))
        wavfile.write(d / f'piece_{v}.wav', FS, (np.clip(mix, -1, 1) * 32767).astype(np.int16))
        clips[v] = mix[56 * FS:134 * FS].copy()
        lu[v] = S.integrated_lufs(clips[v], FS)
    nf, fin = FS // 100, {}
    for v in NAMES:
        y = clips[v] * 10 ** ((min(lu.values()) - lu[v]) / 20)
        y[:nf] *= np.linspace(0, 1, nf)[:, None]
        y[-nf:] *= np.linspace(1, 0, nf)[:, None]
        fin[v] = (y * 32767).astype(np.int16)
    lf = {v: S.integrated_lufs(fin[v] / 32768.0, FS) for v in NAMES}
    Q = ('B1', 'B2')
    out = {'versions': V, 'ref_difference': refdiff, 'manifest': man, 'outcome': {
        'U1': u1 and refdiff <= 1e-3, 'U2': all(V[v]['boundary_AB'] <= 9 and V[v]['boundary_BA'] <= 9 for v in Q),
        'U3': all(V[v]['conflicts'] == 0 and all(V[v]['moved'][s] <= 0.2 * V[v]['notes_per_section'][s] for s in range(3)) for v in Q),
        'U4': all(V[v]['above_min_share'] >= 0.5 for v in Q),
        'U5': all(V[v]['tonic_per_section'][1] >= 4 and V[v]['tonic_per_section'][2] >= 4 and V[v]['last_on_tonic'] for v in Q),
        'U6': all(V[v]['max_leap'] <= 12 for v in Q), 'U7': all(V[v]['same_contour_B'] < 0.6 for v in Q),
        'U8': man['status'] == 'MEASURED' and len(man['renders']) == 4 and all(r['max_gain_reduction_db'] == 0 and r['staged_at_end'] == 0 and r['backpressure_drops'] == 0 for r in man['renders']) and
        all(V[v]['peak_dbfs'] <= -1 for v in NAMES) and max(lf.values()) - min(lf.values()) <= 0.1}}
    if a.clips and out['outcome']['U1'] and out['outcome']['U8']:
        seed = secrets.randbits(32)
        rng = random.Random(seed)
        names = list(NAMES) + [rng.choice(NAMES)]
        rng.shuffle(names)
        c = Path(a.clips)
        c.mkdir(parents=True, exist_ok=True)
        for i, k in enumerate(names, 1):
            wavfile.write(c / f'brano_{i}.wav', FS, fin[k])
        Path(a.key).write_text(f'CHIAVE - non aprire prima di aver risposto\nseed {seed}\n' + '\n'.join(f'brano_{i} = {k}' for i, k in enumerate(names, 1)) + '\n', encoding='utf-8')
    Path(a.out).write_text(json.dumps(out, indent=1, default=lambda v: v.item()) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(out['outcome']), refdiff)
    for v in NAMES:
        print(v, V[v])


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=('write', 'measure'))
    for o in ('plan', 'dir', 'out', 'clips', 'key', 'ref'):
        ap.add_argument('--' + o)
    a = ap.parse_args()
    (write if a.mode == 'write' else measure)(a)
