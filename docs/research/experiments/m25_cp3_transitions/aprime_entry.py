"""B->A' entry variants (docs/research/M2_5_CP3_I5b_BAprime_Harmony_Preregistration.md).

  python -I aprime_entry.py write   --plan <csv> --dir artifacts/cacp/tempo_ramp_v1   (empty the directory of other CSV first)
  cymatica_game --cacp-run warped_plan_render_v1
  python -I aprime_entry.py measure --plan <csv> --dir ... --reference <wav> [--clips <dir> --key <file>]
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
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / 'expert_production_analysis'))
import tempo_ramp as T  # noqa: E402
import stem_analysis as S  # noqa: E402

FS = 48000
B0 = 64.0 + 128 * 60 / 140  # start of A'


def midi(e):
    return int(round(69 + 12 * np.log2(float(e['p']) / 440)))


def variant(ev, v):
    out, changed = [], []
    for e in ev:
        e = dict(e)
        t = e['f'] / FS - B0
        # A' starts on a floored sample of a fractional tempo: half a millisecond of tolerance on the window edges
        in16 = -5e-4 <= t < 8 - 5e-4
        bar2 = 2 - 5e-4 <= t < 4 - 5e-4
        m = midi(e)
        n = m
        if v in 'qs' and bar2 and e['role'] in (1, 2, 3) and m % 12 == 11:
            n -= 1
        if v in 'rs' and in16 and e['role'] == 3:
            n -= 12
        if n != m:
            e['p'] = f'{440 * 2 ** ((n - 69) / 12):.4f}'
            changed.append((e['id'], e['role'], round(t, 3), m, n))
        out.append(e)
    return out, changed


def write(a):
    ev = T.read(a.plan)
    d = Path(a.dir)
    d.mkdir(parents=True, exist_ok=True)
    for v in 'pqrs':
        out, _ = variant(ev, v)
        out.sort(key=lambda e: (e['f'], e['id']))
        with open(d / f'{v}.csv', 'w', newline='\n') as w:
            for e in out:
                w.write(f"{e['id']},{e['role']},{e['f']},{e['d']},{e['p']},{e['v']},{e['t']},{e['off'] or ''}\n")
    print('written')


def conflicts(ev):
    c = 0
    corpo = [(e['f'], (e['off'] or e['f'] + e['d']), midi(e)) for e in ev if e['role'] == 1]
    for e in ev:
        t = e['f'] / FS - B0
        if e['role'] == 3 and -5e-4 <= t < 8 - 5e-4:
            c += any(s <= e['f'] < x and (midi(e) - m) % 12 in (1, 11, 6) for s, x, m in corpo)
    return c


def measure(a):
    ev = T.read(a.plan)
    d = Path(a.dir)
    man = json.loads((d / 'warped_plan_render_v1_manifest.json').read_text())
    out = {'manifest': man, 'changed': {}, 'conflicts': {}}
    ref = T.wav(a.reference)
    clips = {}
    for v in 'pqrs':
        o, ch = variant(ev, v)
        out['changed'][v] = [list(c) for c in ch]
        out['conflicts'][v] = conflicts(o)
        x = T.wav(d / f'{v}.wav')
        if v == 'p':
            n = min(len(ref), len(x), int(182.85 * FS))
            out['U1_max_abs_difference'] = float(np.abs(ref[:n] - x[:n]).max())
        clips[v] = x[int((B0 - 16) * FS):int((B0 - 16) * FS) + 32 * FS]
        if v != 'p':
            p = T.wav(d / 'p.wav')
            k = int((B0 - 0.01) * FS)
            out.setdefault('identical_before_A_prime', {})[v] = bool(np.array_equal(p[:k], x[:k]))
    lu = {k: S.integrated_lufs(x, FS) for k, x in clips.items()}
    out['lufs'] = lu
    out['outcome'] = {'U1': out['U1_max_abs_difference'] <= 1e-3,
                      'U2': all(out['identical_before_A_prime'].values()) and all(len(out['changed'][v]) > 0 for v in 'qrs') and
                      not out['changed']['p'] and all(0 <= c[2] + 1e-3 < 8 for v in 'qrs' for c in out['changed'][v]),
                      'U3': {v: out['conflicts'][v] <= out['conflicts']['p'] for v in 'qrs'},
                      'U4': man['status'] == 'MEASURED' and all(r['max_gain_reduction_db'] == 0 for r in man['renders']) and
                      all(abs(lu[v] - lu['p']) <= 0.5 for v in 'qrs')}
    if a.clips and out['outcome']['U1'] and out['outcome']['U2']:
        seed = secrets.randbits(32)
        names = list('pqrs')
        random.Random(seed).shuffle(names)
        c = Path(a.clips)
        c.mkdir(parents=True, exist_ok=True)
        low, nf = min(lu.values()), FS // 100
        fade = np.linspace(0, 1, nf)[:, None]
        for i, k in enumerate(names, 1):
            y = clips[k] * 10 ** ((low - lu[k]) / 20)
            y[:nf] *= fade
            y[-nf:] *= fade[::-1]
            wavfile.write(c / f'ritorno_{i}.wav', FS, (y * 32767).astype(np.int16))
        Path(a.key).write_text('CHIAVE - non aprire prima di aver risposto\nseed ' + str(seed) + '\n' +
                               '\n'.join(f'ritorno_{i} = {k}' for i, k in enumerate(names, 1)) + '\n', encoding='utf-8')
    print(json.dumps(out, indent=1, default=lambda v: v.item()))


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('mode', choices=('write', 'measure'))
    for o in ('plan', 'dir', 'reference', 'clips', 'key'):
        ap.add_argument('--' + o)
    a = ap.parse_args()
    (write if a.mode == 'write' else measure)(a)
