"""I9 quinquies, the leap of phrase V (docs/research/M2_5_CP3_I9e_V_Leap_Preregistration.md).

  python -I v_leap.py write   --plan <csv> --dir artifacts/cacp/tempo_ramp_v1   (empty directory)
  cymatica_game --cacp-run warped_plan_render_v1
  python -I v_leap.py measure --plan <csv> --dir ... --ref <I9 ter folder> --out <json> [--clips <new dir> --key <new file>]
"""
import argparse
import json
import random
import secrets
from pathlib import Path
import sys

import numpy as np
from scipy.io import wavfile

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import n2_revision as R  # noqa: E402

N, P, T, S, W, BP, FS = R.N, R.P, R.T, R.S, R.W, R.BP, 48000
NAMES, LEN = ('R1', 'V1', 'V2'), 20
VP = (2, 6, 18, 22)
BARS = {b: (2.0 * b if b < 32 else 64.0 + 32 * 240 / 140 + 2.0 * (b - 64)) for b in (10, 26, 74, 90)}
_phrase = R.phrase


def phrase(sec, p, name):
    q = _phrase(sec, p, 'R1')
    if R.FORMS[sec][p] == 'V' and sec != 1:
        if name == 'V1':
            q = [(s, d, {32: 7, 44: 5}.get(s, g)) for s, d, g in q]
        if name == 'V2':
            q = [(s, d, 6 if s == 24 else g) for s, d, g in q]
    return q


R.phrase = phrase  # R.plan builds the melody from this
plan = R.plan


def write(a):
    ev = T.read(a.plan)
    d = Path(a.dir)
    d.mkdir(parents=True, exist_ok=True)
    for n in NAMES:
        x, moved = plan(ev, n)
        R.CM3.rows(d, f'k3_{n}', x)
        print(n, moved)
    R.CM3.rows(d, 'bass_line', R.A.bass_rows(R.A.bass_line(ev, 'g0')))


def vettore(o):
    return sorted((e for e in o if e['role'] == 3), key=lambda e: e['f'])


def measure(a):
    ev, d, ref = T.read(a.plan), Path(a.dir), Path(a.ref)
    man = json.loads((d / 'warped_plan_render_v1_manifest.json').read_text())
    k = {v: T.wav(d / f'k3_{v}.wav') for v in NAMES}
    n = min(len(x) for x in k.values())
    edges = [int(x * FS) for x in (*BP.START, BP.START[2] + 64.0)]
    g = np.full(n, 10 ** (N.GAIN_DB[2] / 20))
    for s in range(3):
        g[edges[s]:edges[s + 1]] = 10 ** (N.GAIN_DB[s] / 20)
    bass = np.column_stack([R.CM3.mono(T.wav(d / 'bass_line.wav'), n)] * 2) * g[:, None]
    old = T.wav(ref / 'k3_R1.wav')
    y1 = (d / 'k3_R1.csv').read_bytes() == (ref / 'k3_R1.csv').read_bytes() and len(old) == len(k['R1'])
    diff = float(np.abs(old - k['R1']).max()) if y1 else None
    y1 = y1 and diff <= 1e-3
    plans = {v: plan(ev, v) for v in NAMES}

    def outside(o):
        return R.notes([e for e in o if not (e['role'] == 3 and P.slot(e['f']) // 64 in VP)])
    V, mix = {}, {}
    for v in NAMES:
        o, moved = plans[v]
        y1 = y1 and outside(o) == outside(plans['R1'][0])
        mix[v] = k[v][:n] + bass
        V[v] = R.sym(ev, o)
        q = vettore(o)
        entry = []
        for p in VP:
            i = next(i for i, e in enumerate(q) if P.slot(e['f']) == p * 64 + 32)
            entry.append([P.midi(q[i - 1]), P.midi(q[i])])
        sp = [[(P.slot(e['f']) % 64, P.midi(e)) for e in q if P.slot(e['f']) // 64 == p] for p in VP]
        base = [(s, P.midi(e)) for e in q for s in [P.slot(e['f']) - 64] if 0 <= s < 64]  # phrase 1 is S'
        V[v].update(moved=moved, entry=entry, differs_from_sp=[sum(x != y for x, y in zip(z, base)) for z in sp],
                    peak_dbfs=float(20 * np.log10(np.abs(mix[v]).max())))
        V[v].pop('rhythm')
    items = [(b, v) for b in BARS for v in NAMES]
    seed = secrets.randbits(32)
    rng = random.Random(seed)
    while True:
        rng.shuffle(items)
        if all(x[0] != y[0] and x[1] != y[1] for x, y in zip(items, items[1:])):
            break
    off = [rng.uniform(9 if b == 90 else 6, 14) for b, _ in items]  # bar 90: the piece ends 12 s after it
    raw = []
    for (b, v), o in zip(items, off):
        s = round((BARS[b] - o) * FS)
        raw.append(mix[v][s:s + LEN * FS].copy())
        assert len(raw[-1]) == LEN * FS
    lu = [S.integrated_lufs(x, FS) for x in raw]
    nf, fin = FS // 2, []
    for x, l in zip(raw, lu):
        x = x * 10 ** ((min(lu) - l) / 20)
        x[:nf] *= np.linspace(0, 1, nf)[:, None]
        x[-nf:] *= np.linspace(1, 0, nf)[:, None]
        fin.append(x)
    lf = [S.integrated_lufs(x, FS) for x in fin]
    peak = max(float(20 * np.log10(np.abs(x).max())) for x in fin)
    C = ('V1', 'V2')
    out = {'versions': V, 'R1_render_difference': diff, 'clips_lufs_spread': max(lf) - min(lf), 'clips_peak_dbfs': peak, 'manifest': man, 'outcome': {
        'Y1': bool(y1),
        'Y2': all(V[v]['conflicts'] == 0 and all(V[v]['moved'][s] <= 0.2 * V[v]['notes_per_section'][s] for s in range(3)) for v in NAMES),
        'Y3': all(V[v]['above_min_share'] >= 0.5 for v in NAMES),
        'Y4': all(abs(y - x) <= lim for v, lim in (('V1', 7), ('V2', 5)) for x, y in V[v]['entry']) and all(V[v]['max_leap'] < 9 for v in C),
        'Y5': all(min(V[v]['differs_from_sp']) >= 2 for v in C),
        'Y6': man['status'] == 'MEASURED' and len(man['renders']) == 4 and all(r['max_gain_reduction_db'] == 0 and r['staged_at_end'] == 0 and r['backpressure_drops'] == 0 for r in man['renders']) and
        all(V[v]['peak_dbfs'] <= -1 for v in NAMES) and peak <= -1 and max(lf) - min(lf) <= 0.1}}
    if a.clips and out['outcome']['Y1'] and out['outcome']['Y6']:
        c = Path(a.clips)
        c.mkdir(parents=True)
        for i, x in enumerate(fin, 1):
            wavfile.write(c / f'estratto_{i:02d}.wav', FS, np.round(x * 32767).astype(np.int16))
        with open(a.key, 'x', encoding='utf-8') as f:
            f.write(f'CHIAVE - non aprire prima di aver risposto\nseed {seed}\n')
            for i, ((b, v), o) in enumerate(zip(items, off), 1):
                f.write(f'estratto_{i:02d} = battuta {b}, {v}, salto a {o:.2f} s\n')
    Path(a.out).write_text(json.dumps(out, indent=1, default=lambda x: x.item()) + '\n', encoding='utf-8', newline='\n')
    print(out['outcome'], diff, out['clips_lufs_spread'], peak)
    for v in NAMES:
        print(v, V[v])


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=('write', 'measure'))
    for o in ('plan', 'dir', 'ref', 'out', 'clips', 'key'):
        ap.add_argument('--' + o)
    a = ap.parse_args()
    (write if a.mode == 'write' else measure)(a)
