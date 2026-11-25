"""I9 quater, clips (docs/research/M2_5_CP3_I9d_Breath_Confirmation_Preregistration.md).

  python -I breath_confirmation.py --dir artifacts/cacp/tempo_ramp_v1 --clips <new dir> --key <new file> --out <json>
The key and the per-clip data stay out of the output and of the JSON.
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
import new_motif as N  # noqa: E402

FS, LEN = 48000, 24
BARS = {23: 46.0, 87: 64.0 + 32 * 240 / 140 + 46.0}
ap = argparse.ArgumentParser()
for o in ('dir', 'clips', 'key', 'out'):
    ap.add_argument('--' + o, required=True)
a = ap.parse_args()
w = {v: N.T.wav(Path(a.dir) / f'piece_{v}.wav') for v in ('Y', 'R1')}
seed = secrets.randbits(32)
rng = random.Random(seed)
items = []
for b in BARS:
    for v in w:
        while True:
            o = [rng.uniform(8, 14) for _ in range(2)]
            if abs(o[0] - o[1]) >= 2:
                break
        items += [(b, v, x) for x in o]
while True:
    rng.shuffle(items)
    if all(x[:2] != y[:2] for x, y in zip(items, items[1:])):
        break
z1, raw = True, []
for b, v, o in items:
    s = round((BARS[b] - o) * FS)
    q, r = (w[k][s:s + LEN * FS] for k in ('Y', 'R1'))
    assert len(q) == len(r) == LEN * FS
    c, e = round(o * FS), round((o + 2) * FS)
    z1 &= bool(np.array_equal(q[:c], r[:c]) and not np.array_equal(q[c:e], r[c:e]))
    raw.append((q if v == 'Y' else r).copy())
lu = [N.S.integrated_lufs(x, FS) for x in raw]
nf, fin = FS // 2, []
for x, l in zip(raw, lu):
    x = x * 10 ** ((min(lu) - l) / 20)
    x[:nf] *= np.linspace(0, 1, nf)[:, None]
    x[-nf:] *= np.linspace(1, 0, nf)[:, None]
    fin.append(x)
lf = [N.S.integrated_lufs(x, FS) for x in fin]
peak = max(float(20 * np.log10(np.abs(x).max())) for x in fin)
out = {'Z1': z1, 'Z2': max(lf) - min(lf) <= 0.1 and peak <= -1, 'lufs_spread': max(lf) - min(lf), 'peak_dbfs': peak, 'clips': len(fin)}
if out['Z1'] and out['Z2']:
    c = Path(a.clips)
    c.mkdir(parents=True)
    for i, x in enumerate(fin, 1):
        wavfile.write(c / f'estratto_{i}.wav', FS, np.round(x * 32767).astype(np.int16))
    with open(a.key, 'x', encoding='utf-8') as f:
        f.write(f'CHIAVE - non aprire prima di aver risposto\nseed {seed}\n')
        for i, (b, v, o) in enumerate(items, 1):
            f.write(f'estratto_{i} = battuta {b}, {v}, inizio {o:.2f} s, fine {o + 2:.2f} s\n')
Path(a.out).write_text(json.dumps(out, indent=1) + '\n', encoding='utf-8', newline='\n')
print(out)
