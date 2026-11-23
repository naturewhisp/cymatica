"""Repeated blind pairs at A->B (docs/research/M2_5_CP3_I4_I6c_Boundary_Pairs_Preregistration.md).

  python -I boundary_pairs.py --dir artifacts/cacp/tempo_ramp_v1 --clips <dir> --key <file> --out <json>   (needs b.wav and c.wav)
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
for q in (HERE, HERE.parent / 'm25_cp3_transitions', HERE.parent / 'expert_production_analysis'):
    sys.path.insert(0, str(q))
import stem_analysis as S  # noqa: E402
import tempo_ramp as T  # noqa: E402

FS = 48000


def main():
    ap = argparse.ArgumentParser()
    for o in ('dir', 'clips', 'key', 'out'):
        ap.add_argument('--' + o, required=True)
    a = ap.parse_args()
    nf = FS // 100
    fade = np.linspace(0, 1, nf)[:, None]
    half, lufs = {}, {}
    for v in 'bc':
        x = T.wav(Path(a.dir) / f'{v}.wav')[60 * FS:68 * FS].copy()
        x[:nf] *= fade
        x[-nf:] *= fade[::-1]
        half[v] = (x * 32767).astype(np.int16)
        lufs[v] = S.integrated_lufs(x, FS)
    seed = secrets.randbits(32)
    rng = random.Random(seed)
    pairs = [('c', 'b')] * 3 + [('b', 'c')] * 3 + [('b', 'b'), ('c', 'c')]
    rng.shuffle(pairs)
    gap = np.zeros((FS, half['b'].shape[1]), dtype=np.int16)
    c = Path(a.clips)
    c.mkdir(parents=True, exist_ok=True)
    q1 = q2 = True
    for i, (x, y) in enumerate(pairs, 1):
        z = np.concatenate([half[x], gap, half[y]])
        n = len(half[x])
        same = bool((z[:n] == z[n + FS:]).all())
        q1 &= same if x == y else True
        q2 &= not same if x != y else True
        wavfile.write(c / f'coppia_{i}.wav', FS, z)
    q2 &= abs(lufs['b'] - lufs['c']) <= 0.2
    peak = int(max(np.abs(h.astype(int)).max() for h in half.values()))
    q3 = len(pairs) == 8 and sorted(pairs) == sorted([('c', 'b')] * 3 + [('b', 'c')] * 3 + [('b', 'b'), ('c', 'c')]) and peak < 32767
    Path(a.key).write_text(f'CHIAVE - non aprire prima di aver risposto\nseed {seed}\n' +
                           '\n'.join(f'coppia_{i} = {x} poi {y}' for i, (x, y) in enumerate(pairs, 1)) + '\n', encoding='utf-8')
    out = {'Q1': q1, 'Q2': bool(q2), 'Q3': bool(q3), 'lufs': lufs, 'peak_int16': peak, 'seconds_per_pair': (2 * len(half['b']) + FS) / FS}
    Path(a.out).write_text(json.dumps(out, indent=1, default=lambda v: v.item()) + '\n', encoding='utf-8', newline='\n')
    print(out)


if __name__ == '__main__':
    main()
