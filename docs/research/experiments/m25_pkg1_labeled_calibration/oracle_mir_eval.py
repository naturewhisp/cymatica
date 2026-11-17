"""V5 oracle of measurement package 1: mir_eval onset F-measure on the lists exported by acao02_v1.

Run with the pinned oracle environment (tools/pyenv, requirements-oracle.txt):
  tools/pyenv/.venv/Scripts/python.exe -I docs/research/experiments/m25_pkg1_labeled_calibration/oracle_mir_eval.py artifacts/cacp/acao02_v1_oracle
For each role: same number of one-to-one matches at 0.05 s as CYMATICA, hence the same F to 1e-9.
"""
import sys
from pathlib import Path

import mir_eval
import numpy as np

d = Path(sys.argv[1])
ok = True
for role in ('pulso', 'corpo', 'trama', 'vettore'):
    p = d / f'{role}.csv'
    refs, est, ours = [], [], None
    for line in p.read_text().splitlines()[1:]:
        kind, value = line.split(',')
        if kind == 'reference':
            refs.append(float(value))
        elif kind == 'estimate':
            est.append(float(value))
        elif kind == '# matches':
            ours = int(value)
    refs, est = np.sort(np.array(refs)), np.sort(np.array(est))
    matching = mir_eval.util.match_events(refs, est, 0.05)
    f, prec, rec = mir_eval.onset.f_measure(refs, est, window=0.05)
    n = len(matching)
    our_p = ours / len(est) if len(est) else 0.0
    our_r = ours / len(refs) if len(refs) else 0.0
    our_f = 2 * our_p * our_r / (our_p + our_r) if (our_p + our_r) else 0.0
    agree = (n == ours) and abs(f - our_f) <= 1e-9
    ok = ok and agree
    print(f'{role:8s} refs {len(refs):4d} est {len(est):4d} matches mir_eval {n:4d} cymatica {ours:4d} '
          f'F mir_eval {f:.12f} cymatica {our_f:.12f} -> {"AGREE" if agree else "DISAGREE"}')
print('V5:', 'AGREE' if ok else 'DISAGREE')
