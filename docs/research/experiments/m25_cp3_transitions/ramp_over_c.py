"""Tempo ramp over version c, repeated pairs (docs/research/M2_5_CP3_I5e_Ramp_Over_C_Preregistration.md).

  python -I ramp_over_c.py write   --plan <csv> --dir artifacts/cacp/tempo_ramp_v1   (empty directory of other CSV)
  cymatica_game --cacp-run warped_plan_render_v1
  python -I ramp_over_c.py measure --dir ... --cref <c.wav of the breath test> --clips <dir> --key <file> --out <json>
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
for q in (HERE, HERE.parent / 'm25_cp3_melody', HERE.parent / 'expert_production_analysis'):
    sys.path.insert(0, str(q))
import breath_variants as B  # noqa: E402
import stem_analysis as S  # noqa: E402
import tempo_ramp as T  # noqa: E402

FS = 48000
K = {'n': 0, 'r8': 8, 'r16': 16}


def beat_lengths(v):
    ln = [0.5] * 128 + [60 / 140] * 128 + [0.5] * 128
    k = K[v]
    if k:
        s = 20 / k
        ln[128 - k:128] = [60 * np.log((120 + s * (i + 1)) / (120 + s * i)) / s for i in range(k)]
    return ln


def ramp_time(v):
    ln, k = beat_lengths(v), K[v]
    cum = np.concatenate([[0.0], np.cumsum(ln)])

    def f(beat):
        i = min(int(np.floor(beat + 1e-9)), len(ln) - 1)
        x = beat - i
        if k and 128 - k <= i < 128:
            q, s = i - (128 - k), 20 / k
            return cum[i] + 60 * np.log((120 + s * (q + x)) / (120 + s * q)) / s
        return cum[i] + x * ln[i]
    return f, cum


def write(a):
    ev = B.variant(T.read(a.plan), 'c')
    d = Path(a.dir)
    d.mkdir(parents=True, exist_ok=True)
    for v in K:
        f, _ = ramp_time(v)

        def fr(x):
            return x if v == 'n' else int(round(f(T.original_beat(x)) * FS))
        rows = []
        for e in ev:
            s = fr(e['f'])
            n = max(fr(e['f'] + e['d']) - s, 1) if e['d'] else 0
            rows.append((s, e['id'], e['role'], n, e['p'], e['v'], e['t'], fr(e['off']) if e['off'] else ''))
        rows.sort(key=lambda x: (x[0], x[1]))
        with open(d / f'{v}.csv', 'w', newline='\n') as w:
            for s, i, r, n, p, ve, t, o in rows:
                w.write(f'{i},{r},{s},{n},{p},{ve},{t},{o}\n')


def measure(a):
    d = Path(a.dir)
    man = json.loads((d / 'warped_plan_render_v1_manifest.json').read_text())
    x = {v: T.wav(d / f'{v}.wav') for v in K}
    cref = T.wav(a.cref)
    m = min(len(cref), len(x['n']))
    r1 = {'n_vs_c': float(np.abs(cref[:m] - x['n'][:m]).max())}
    for v in ('r8', 'r16'):
        t = int(((128 - K[v]) * 0.5) * FS)
        r1[f'{v}_before_ramp'] = float(np.abs(x[v][:t] - x['n'][:t]).max())
    jump = {v: float(max(abs(p / q - 1) for q, p in zip(beat_lengths(v)[:200], beat_lengths(v)[1:200]))) for v in K}
    nf = FS // 100
    fade = np.linspace(0, 1, nf)[:, None]
    half, lufs = {}, {}
    for v in K:
        b = ramp_time(v)[1][128]
        s = int((b - 10) * FS)
        y = x[v][s:s + 14 * FS].copy()
        y[:nf] *= fade
        y[-nf:] *= fade[::-1]
        half[v] = (y * 32767).astype(np.int16)
        lufs[v] = S.integrated_lufs(y, FS)
    seed = secrets.randbits(32)
    plan = [('n', 'r8')] * 3 + [('r8', 'n')] * 3 + [('r8', 'r16')] * 2 + [('r16', 'r8')] * 2 + [('n', 'n'), ('r8', 'r8')]
    pairs = list(plan)
    random.Random(seed).shuffle(pairs)
    peak = int(max(np.abs(h.astype(int)).max() for h in half.values()))
    outcome = {
        'R1': all(q <= 1e-3 for q in r1.values()),
        'R2': bool(abs(jump['n'] - 1 / 7) < 0.01 and jump['r8'] <= 0.025 and jump['r16'] <= 0.013),
        'R3': man['status'] == 'MEASURED' and len(man['renders']) == 3 and
        all(r['max_gain_reduction_db'] == 0 and r['staged_at_end'] == 0 and r['backpressure_drops'] == 0 for r in man['renders']),
        'R4': max(lufs.values()) - min(lufs.values()) <= 0.5 and sorted(pairs) == sorted(plan) and peak < 32767}
    out = {'manifest': man, 'R1_differences': r1, 'R2_jump': jump, 'lufs': lufs, 'peak_int16': peak,
           'boundary_s': {v: float(ramp_time(v)[1][128]) for v in K}, 'outcome': outcome}
    if all(outcome.values()):
        c = Path(a.clips)
        c.mkdir(parents=True, exist_ok=True)
        gap = np.zeros((FS, 2), dtype=np.int16)
        for i, (p, q) in enumerate(pairs, 1):
            wavfile.write(c / f'rampa_{i:02d}.wav', FS, np.concatenate([half[p], gap, half[q]]))
        Path(a.key).write_text(f'CHIAVE - non aprire prima di aver risposto\nseed {seed}\n' +
                               '\n'.join(f'rampa_{i:02d} = {p} poi {q}' for i, (p, q) in enumerate(pairs, 1)) + '\n', encoding='utf-8')
    Path(a.out).write_text(json.dumps(out, indent=1, default=lambda v: v.item()) + '\n', encoding='utf-8', newline='\n')
    print(outcome, r1, jump, lufs, peak, out['boundary_s'])


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=('write', 'measure'))
    for o in ('plan', 'dir', 'cref', 'clips', 'key', 'out'):
        ap.add_argument('--' + o)
    a = ap.parse_args()
    (write if a.mode == 'write' else measure)(a)
