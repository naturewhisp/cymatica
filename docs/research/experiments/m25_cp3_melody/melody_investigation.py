"""Measures M1-M6 of docs/research/M2_5_CP3_I6_Melody_Investigation_Preregistration.md.

Usage: python -I melody_investigation.py --tokens artifacts/cacp/form_investigation_v1/plan_tokens.csv
"""
import argparse
import csv
import json
from collections import defaultdict

import numpy as np

TONIC = [2, 7, 2]


class Ppm:
    """Short-term model, orders 2..0, PPM-C escape, symbols = intervals; unseen symbols share a uniform escape mass."""
    N = 49  # intervals -24..24

    def __init__(self):
        self.c = defaultdict(lambda: defaultdict(int))

    def prob(self, ctx, s):
        p = 1.0
        seen = set()
        for o in (2, 1, 0):
            if len(ctx) < o:
                continue
            d = self.c[(o, tuple(ctx[len(ctx) - o:]))]
            avail = {k: v for k, v in d.items() if k not in seen}
            if not avail:
                continue
            tot, u = sum(avail.values()), len(avail)
            if s in avail:
                return p * avail[s] / (tot + u)
            p *= u / (tot + u)
            seen |= set(avail)
        return p / max(1, self.N - len(seen))

    def add(self, ctx, s):
        for o in (0, 1, 2):
            if len(ctx) >= o:
                self.c[(o, tuple(ctx[len(ctx) - o:]))][s] += 1


def analyse(tok):
    tok = sorted(tok)
    bars = [[m for on, m in tok if on // 16 == b] for b in range(96)]
    contour = [tuple(int(np.sign(y - x)) for x, y in zip(q, q[1:])) for q in bars]
    figure = [tuple(y - x for x, y in zip(q, q[1:])) for q in bars]
    same = [contour[p * 4 + k] == contour[p * 4] for p in range(24) for k in (1, 2, 3)]
    samef = [figure[p * 4 + k] == figure[p * 4] for p in range(24) for k in (1, 2, 3)]
    distinct = [len(set(contour[s * 32:s * 32 + 32])) for s in range(3)]
    # run of consecutive phrases sharing the opening contour
    model, ctx, ic = Ppm(), [], []
    prev = None
    for on, m in tok:
        if prev is not None:
            s = max(-24, min(24, m - prev))
            ic.append((on // 16, -np.log2(model.prob(ctx, s))))
            model.add(ctx, s)
            ctx = (ctx + [s])[-2:]
        prev = m
    half = []
    for s in range(3):
        a = [v for b, v in ic if s * 32 <= b < s * 32 + 16]
        c = [v for b, v in ic if s * 32 + 16 <= b < s * 32 + 32]
        half.append({'first': float(np.mean(a)), 'second': float(np.mean(c)), 'delta': float(np.mean(c) - np.mean(a))})
    ends = {'even': [], 'odd': []}
    for p in range(24):
        ends['even' if p % 8 % 2 == 0 else 'odd'].append(bars[p * 4 + 3][-1] % 12 == TONIC[p // 8])
    e = {k: float(np.mean(v)) for k, v in ends.items()}
    breath = sum(len(bars[p * 4 + 3]) < min(len(bars[p * 4 + k]) for k in range(3)) for p in range(24))
    return {'same_contour_as_bar1': float(np.mean(same)), 'same_figure_as_bar1': float(np.mean(samef)),
            'distinct_contours_per_section': distinct, 'ic_halves': half, 'ic_mean': float(np.mean([v for _, v in ic])),
            'phrase_end_on_tonic': e, 'M6_breath_phrases': breath, 'notes_per_bar': sorted({len(q) for q in bars})}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--tokens', required=True)
    a = ap.parse_args()
    pl = defaultdict(list)
    for r in csv.DictReader(open(a.tokens)):
        if r['role'] == '3':
            pl[r['plan']].append((int(r['onset']), int(r['midi'])))
    res = {k: analyse(v) for k, v in pl.items()}
    seeds = [res[str(s)] for s in range(1, 65)]
    med = lambda f: float(np.median([f(x) for x in seeds]))  # noqa: E731
    r = res['ref']
    out = {'reference': r, 'seed_34': res['34'],
           'seeds': {'M2_same_contour_median': med(lambda x: x['same_contour_as_bar1']),
                     'same_contour_min_max': [min(x['same_contour_as_bar1'] for x in seeds), max(x['same_contour_as_bar1'] for x in seeds)],
                     'M3_distinct_mean_median': med(lambda x: np.mean(x['distinct_contours_per_section'])),
                     'M6_breath_total': sum(x['M6_breath_phrases'] for x in seeds), 'ic_mean_median': med(lambda x: x['ic_mean'])},
           'outcome': {'M1': r['same_contour_as_bar1'] >= 0.70, 'M2': med(lambda x: x['same_contour_as_bar1']) >= 0.60,
                       'M3': float(np.mean(r['distinct_contours_per_section'])) <= 6,
                       'M4': sum(h['delta'] < 0 for h in r['ic_halves']) >= 2,
                       'M5': abs(r['phrase_end_on_tonic']['even'] - r['phrase_end_on_tonic']['odd']) < 0.25,
                       'M6': r['M6_breath_phrases'] == 0}}
    print(json.dumps(out, indent=1, default=lambda x: x.item()))


if __name__ == '__main__':
    main()
