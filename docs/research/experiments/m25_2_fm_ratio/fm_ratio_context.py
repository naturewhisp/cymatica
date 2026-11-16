"""Criteria C1-C3 of the pre-registered FM ratio choice (docs/research/M2_5_2_FM_Ratio_Preregistration.md).

Sethares interference model on theoretical spectra, same functions as m25_ensemble_baseline/fm_sethares.py.
Usage: python -I fm_ratio_context.py docs/research/experiments/cacp_methods/fixture_intents_seed1337.csv
CSV roles: 0 Pulso, 1 Corpo, 2 Trama, 3 Vettore.
"""
import csv
import math
import sys
from functools import lru_cache

CANDIDATES = [('1.88 (attuale)', 1.88), ('1.0', 1.0), ('2.0', 2.0), ('sqrt2', 2 ** 0.5)]
SAW_REFERENCE = 0.028


@lru_cache(maxsize=None)
def J(n, x, N=2000):
    s = 0.0
    for k in range(N):
        t = (k + .5) * math.pi / N
        s += math.cos(n * t - x * math.sin(t))
    return s / N


def fm(c, m, I, kmax=12):
    d = {}

    def add(f, a):
        if f < 0:
            f, a = -f, -a
        key = round(f, 3)
        d[key] = d.get(key, 0) + a
    add(c, J(0, I))
    for n in range(1, kmax):
        j = J(n, I)
        add(c + n * m, j)
        add(c - n * m, ((-1) ** n) * j)
    return [(f, abs(a)) for f, a in sorted(d.items()) if abs(a) > 1e-3 and f > 0]


def diss(sp):
    Dstar, S1, S2, C1, C2, A1, A2 = .24, .0207, 18.96, 5, -5, -3.51, -5.75
    s = 0.0
    sp = sorted(sp)
    for i in range(len(sp)):
        for j in range(i + 1, len(sp)):
            f1, a1 = sp[i]
            f2, a2 = sp[j]
            S = Dstar / (S1 * f1 + S2)
            d = f2 - f1
            s += min(a1, a2) * (C1 * math.exp(A1 * S * d) + C2 * math.exp(A2 * S * d))
    return s


def saw(f, n=10):
    return [(f * k, 1 / k) for k in range(1, n + 1)]


rows = list(csv.DictReader(open(sys.argv[1], newline='')))
# Open pads have duration 0 and an explicit '#noteoff,<event_id>,<role>,<frame>' row
noteoff = {int(r['section']): int(r['role']) for r in rows if r['event_id'] == '#noteoff'}
notes = []
for r in rows:
    if r['event_id'].startswith('#'):
        continue
    start, dur = int(r['sample_frame']), int(r['duration_frames'])
    if dur == 0:
        dur = noteoff[int(r['event_id'])] - start
    notes.append((int(r['role']), start, dur, float(r['pitch_hz'])))
trama = [n for n in notes if n[0] == 2]


def sounding(role, frame):
    return [p for (r, s, d, p) in notes if r == role and s <= frame < s + d]


contexts = [(p, sounding(1, s), sounding(3, s)) for (_, s, _, p) in trama]
print(f'Trama notes: {len(trama)}; with Corpo sounding at onset: {sum(1 for c in contexts if c[1])}')
print(f'Trama pitches (Hz): {sorted(set(round(p, 2) for p, _, _ in contexts))}')

results = {}
for name, r in CANDIDATES:
    c1 = sum(diss(fm(p, p * r, 3.0)) for p, _, _ in contexts) / len(contexts)
    f = lambda k: fm(440 * k, 440 * k * r, 1.5)
    p5, p8, tt = diss(f(1) + f(2 ** (7 / 12))), diss(f(1) + f(2)), diss(f(1) + f(2 ** .5))
    c2 = p5 < tt and p8 < tt
    c3s, c3cv = [], []
    for I in (1.5, 3.0):
        for p, corpo, vettore in contexts:
            t = fm(p, p * r, I)
            base = [x for q in corpo for x in saw(q)]
            c3s.append(diss(base + t) - diss(base) - diss(t))
            base2 = base + [x for q in vettore for x in saw(q)]
            c3cv.append(diss(base2 + t) - diss(base2) - diss(t))
    c3 = sum(c3s) / len(c3s)
    c3b = sum(c3cv) / len(c3cv)
    results[name] = (c1, c2, c3)
    print(f'{name:16s} C1 {c1:.4f} ({"pass" if c1 <= SAW_REFERENCE else "FAIL"})  '
          f'C2 P5 {p5:.3f} P8 {p8:.3f} TT {tt:.3f} ({"pass" if c2 else "FAIL"})  C3 {c3:.4f}  [C3 Corpo+Vettore {c3b:.4f}]')

passing = [(n, v) for n, v in results.items() if v[0] <= SAW_REFERENCE and v[1]]
if not passing:
    print('choice: none passes C1 and C2 -> ratio stays 1.88')
else:
    best = min(v[2] for _, v in passing)
    ties = [n for n, v in passing if v[2] <= best * 1.05]
    print('passing C1+C2:', [n for n, _ in passing], '; within 5% of best C3:', ties)
    print('choice (smallest change of K among ties):', min(ties, key=lambda n: abs(dict(CANDIDATES)[n] - 1.88)))
