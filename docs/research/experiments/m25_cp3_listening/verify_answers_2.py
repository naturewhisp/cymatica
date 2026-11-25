"""Post-hoc checks on the second CP-3 expert answers, on the plan of piece Y. Nothing here was pre-registered.

  python -I verify_answers_2.py --cacp artifacts/cacp --out verify_answers_2.json
Plan rows: id, role, frame, duration, Hz (roles 0 Pulso, 1 Corpo, 2 Trama, 3 Vettore); bar 0-based, B = 32..63.
"""
import argparse
import csv
import json
from collections import Counter
from pathlib import Path

import numpy as np

FS, START, BAR = 48000, (0.0, 64.0, 64.0 + 32 * 240 / 140), (2.0, 240 / 140, 2.0)
TONIC = (2, 7, 2)  # D, G, D
ap = argparse.ArgumentParser()
for o in ('cacp', 'out'):
    ap.add_argument('--' + o, required=True)
a = ap.parse_args()


def bar_of(t):
    s = max(i for i in range(3) if t >= START[i] - 1e-3)
    return s * 32 + (t - START[s] + 1e-3) / BAR[s]


def plan(p):
    ev = []
    for r in csv.reader(open(p)):
        if len(r) < 5 or not r[0].isdigit() or int(r[1]) not in (1, 2, 3):
            continue
        t = int(r[2]) / FS
        ev.append({'role': int(r[1]), 't': t, 'e': t + int(r[3]) / FS, 'm': round(69 + 12 * np.log2(float(r[4]) / 440)), 'bar': int(bar_of(t))})
    return ev


def stats(ev, bars):
    v = [x for x in ev if x['role'] == 3 and x['bar'] in bars]
    if not v:
        return None
    octave = unison = 0
    for n in v:
        co = [c['m'] for c in ev if c['role'] == 1 and c['t'] - 1e-3 <= n['t'] < c["e"] - 1e-3]
        unison += n['m'] in co
        octave += (n['m'] - 12) in co
    same_pc = sum(any((n['m'] - c['m']) % 12 == 0 for c in ev if c['role'] == 1 and c['t'] - 1e-3 <= n['t'] < c["e"] - 1e-3) for n in v)
    s = min(bars) // 32
    return {'vettore_notes': len(v), 'vettore_median_midi': float(np.median([n['m'] for n in v])),
            'share_same_pitch_class_as_corpo': same_pc / len(v), 'share_exactly_one_octave_above_corpo': octave / len(v),
            'share_unison_with_corpo': unison / len(v), 'share_on_section_tonic': sum(n['m'] % 12 == TONIC[s] for n in v) / len(v),
            'distinct_pitch_classes_all_roles': len({x['m'] % 12 for x in ev if x['bar'] in bars})}


out = {'dip_1_44_bar_0_based': bar_of(104.0)}
for name, f in (('Y', 'tempo_ramp_v1/k3_B2.csv'), ('today', 'tempo_ramp_v1_i9/k3_p.csv')):
    ev = plan(Path(a.cacp) / f)
    out[name] = {'sections': {'ABC'[s]: stats(ev, range(32 * s, 32 * s + 32)) for s in range(3)},
                 'four_bar_windows': {f'{b}-{b + 3}': stats(ev, range(b, b + 4)) for b in range(0, 96, 4)},
                 'bars_48_59': {b: {'vettore': [x['m'] for x in ev if x['role'] == 3 and x['bar'] == b],
                                    'corpo': sorted({x['m'] for x in ev if x['role'] == 1 and x['bar'] == b}),
                                    'trama_notes': sum(x['role'] == 2 and x['bar'] == b for x in ev)} for b in range(48, 60)}}
Path(a.out).write_text(json.dumps(out, indent=1) + '\n', encoding='utf-8', newline='\n')
print('dip bar', out['dip_1_44_bar_0_based'])
for n in ('Y', 'today'):
    print(n)
    for k, s in (*out[n]['sections'].items(), *out[n]['four_bar_windows'].items()):
        print(k, s and [round(x, 2) for x in s.values()])
for b, r in out['Y']['bars_48_59'].items():
    print(b, r)
