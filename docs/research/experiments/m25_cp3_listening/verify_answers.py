"""Post-hoc checks on the CP-3 expert answers: package audio against its sources, and kick/bass alignment in piece Z.

  python -I verify_answers.py --pkg <package folder> --cacp artifacts/cacp --plan <csv> --out <json>
Nothing here was pre-registered.
"""
import argparse
import json
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import cp3_material as CM3  # noqa: E402
import make_cp3_package as MP  # noqa: E402

T, BP, P, FS = CM3.T, CM3.BP, CM3.P, 48000
ap = argparse.ArgumentParser()
for o in ('pkg', 'cacp', 'plan', 'out'):
    ap.add_argument('--' + o, required=True)
a = ap.parse_args()
au = Path(a.pkg) / 'audio'
out = {'pairs': {}}
for n in range(1, 8):
    x, y = T.wav(au / f'coppia_{n}_prima.wav'), T.wav(au / f'coppia_{n}_seconda.wav')
    d = x - y
    out['pairs'][n] = {'max_abs_difference': float(np.abs(d).max()),
                       'difference_rms_rel_db': float(10 * np.log10(max((d ** 2).mean(), 1e-14) / (x ** 2).mean()))}
m = MP.material(Path(a.cacp))
x = T.wav(au / 'coppia_2_prima.wav')


def corr(u, v):
    u, v = u.mean(axis=1), v.mean(axis=1)
    k = min(len(u), len(v))
    return float((u[:k] * v[:k]).sum() / np.sqrt((u[:k] ** 2).sum() * (v[:k] ** 2).sum()))


out['pair2_first_correlation'] = {name: corr(x, arr) for arr, name in m['M']}
ev = T.read(a.plan)
for tag, plan in (('today', ev), ('z', CM3.z_plan(ev))):
    k = CM3.kicks_of(plan)
    bass = BP.line(k)[0]
    ln = [e for e in bass if e['phrase'] not in BP.PEDAL]
    r = {}
    for s in range(3):
        b = [e for e in ln if e['bar'] // 32 == s]
        hit = sum(BP.coincident(e['t'], s, k) for e in b)
        bt = np.array([e['t'] for e in bass])
        ks = [q for q in k if BP.START[s] <= q < (BP.START + [1e9])[s + 1]]
        beat = BP.BAR[s] / 4
        on = [min((q - BP.START[s]) % beat, beat - (q - BP.START[s]) % beat) < beat / 16 for q in ks]
        bon = [min((e['t'] - BP.START[s]) % beat, beat - (e['t'] - BP.START[s]) % beat) < beat / 16 for e in b]
        r['ABC'[s]] = {'bass_onsets_on_kick': hit / len(b), 'kicks_with_bass_onset': float(np.mean([np.abs(bt - q).min() <= BP.BAR[s] / 32 for q in ks])),
                       'kicks_on_beat': float(np.mean(on)), 'bass_onsets_on_beat': float(np.mean(bon))}
    out[tag] = r
z = CM3.z_plan(ev)
out['dip_bars'] = {}
for t in (47, 84, 123, 129):
    s = max(i for i in range(3) if t >= BP.START[i])
    out['dip_bars'][t] = s * 32 + (t - BP.START[s]) / BP.BAR[s]
Path(a.out).write_text(json.dumps(out, indent=1) + '\n', encoding='utf-8', newline='\n')
print(json.dumps(out, indent=1))
