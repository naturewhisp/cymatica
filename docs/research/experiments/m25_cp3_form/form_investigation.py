"""Measures G1-G8 of docs/research/M2_5_CP3_I4_Form_Investigation_Preregistration.md (aggregate statistics only).

Usage: python -I form_investigation.py --tokens artifacts/cacp/form_investigation_v1/plan_tokens.csv --midi <expert MIDI dir>
"""
import argparse
import csv
import json
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'expert_production_analysis'))
import midi_analysis as M  # noqa: E402

ROLES = ['pulso', 'corpo', 'trama', 'vettore']
START = [0.0, 64.0, 64.0 + 32 * 4 * 60 / 140]
BAR = [2.0, 4 * 60 / 140, 2.0]
DROPS = [15, 92, 120, 140, 158, 166]  # 0:15 1:32 2:00 2:20 2:38 2:46


def bars_of(tokens):
    """role -> list of 96 (rhythm, content)"""
    out = {}
    for r in range(4):
        rh, ct = [set() for _ in range(96)], [set() for _ in range(96)]
        for on, midi in tokens.get(r, []):
            b = on // 16
            if 0 <= b < 96:
                rh[b].add(on % 16)
                ct[b].add((on % 16, midi))
        out[r] = [(frozenset(a), frozenset(c)) for a, c in zip(rh, ct)]
    return out


def share(x):
    return (sum(x) / len(x)) if x else None


def changes(b):
    """bars where some role goes from playing to silent or back"""
    return sorted({k for r in range(4) for k in range(1, 96) if bool(b[r][k][0]) != bool(b[r][k - 1][0])})


def windows(ch):
    s = set(ch)
    return [any(k in s for k in range(w, w + 8)) for w in range(0, 96, 8)]


def analyse(b):
    res = {}
    for r, name in enumerate(ROLES):
        same, fill, fill_n, phr, samec = [], 0, 0, [], []
        for p in range(24):
            q = [b[r][p * 4 + k] for k in range(4)]
            for k in range(1, 4):
                if q[k][0] and q[k - 1][0]:
                    same.append(q[k][0] == q[k - 1][0])
                    samec.append(q[k][1] == q[k - 1][1])
            if all(x[0] for x in q):
                fill_n += 1
                fill += all(q[3][0] != q[k][0] for k in range(3))
            if p % 8:
                a = [b[r][(p - 1) * 4 + k][0] for k in range(4)]
                c = [x[0] for x in q]
                if any(a) and any(c):
                    phr.append(a != c)
        # longest run of bars with the same rhythm (bars where the role plays)
        run = best = 0
        for k in range(96):
            run = run + 1 if k and b[r][k][0] and b[r][k][0] == b[r][k - 1][0] else (1 if b[r][k][0] else 0)
            best = max(best, run)
        res[name] = {'G1_same_rhythm_within_phrase': share(same), 'same_content_within_phrase': share(samec),
                     'G2_phrases_with_bar4_fill': fill, 'phrases_playing_all_bars': fill_n,
                     'G3_phrase_rhythm_changes': share(phr), 'longest_same_rhythm_run_bars': best}
    ch = changes(b)
    w = windows(ch)
    res['change_bars'] = ch
    res['windows_share'] = share(w)
    return res


def expert(midi):
    out = {}
    for name, pitches in (('Kick & Snare', None), ('808_Bass', None)):
        m = M.read_smf(midi / f'{name}.mid')
        six, bar = m['ppq'] / 4, m['ppq'] * 4
        bars = {}
        for n in m['notes']:
            bars.setdefault(n['tick'] // bar, set()).add((round((n['tick'] % bar) / six) % 16, n['pitch']))
        ks = sorted(bars)
        eq = {k: bars[k] == bars[k - 1] for k in ks if k - 1 in bars}
        last = [not v for k, v in eq.items() if k % 4 == 3]
        rest = [not v for k, v in eq.items() if k % 4 != 3]
        first = [not v for k, v in eq.items() if k % 4 == 0]
        out[name] = {'bars_with_previous': len(eq), 'G4_same_as_previous': share(list(eq.values())),
                     'G5_last_of_4_differs': share(last), 'G5_other_bars_differ': share(rest), 'first_of_4_differs': share(first)}
    return out


def expert_lags(midi):
    """Post-hoc, declared after G4 saturated at 0: share of drum bars equal to the bar `lag` bars earlier (rhythm), and,
    by position in the group, the share that differs."""
    m = M.read_smf(midi / 'Kick & Snare.mid')
    six, bar = m['ppq'] / 4, m['ppq'] * 4
    out = {}
    for name, sel in (('all', {36, 37, 38}), ('kick', {36}), ('snare', {38})):
        b = {}
        for n in m['notes']:
            if n['pitch'] in sel:
                b.setdefault(n['tick'] // bar, set()).add(round((n['tick'] % bar) / six) % 16)
        out[name] = {str(g): share([b[k] == b[k - g] for k in b if k - g in b]) for g in (1, 2, 4, 8, 16)}
        if name == 'all':
            pos = {}
            for k in b:
                if k - 8 in b:
                    pos.setdefault(k % 8, []).append(b[k] != b[k - 8])
            out['differs_from_8_bars_earlier_by_position'] = {str(k): share(v) for k, v in sorted(pos.items())}
            out['bars_per_position'] = {str(k): len(v) for k, v in sorted(pos.items())}
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--tokens', required=True)
    ap.add_argument('--midi', required=True)
    a = ap.parse_args()
    plans = {}
    for r in csv.DictReader(open(a.tokens)):
        plans.setdefault(r['plan'], {}).setdefault(int(r['role']), []).append((int(r['onset']), int(r['midi'])))
    b = {k: bars_of(v) for k, v in plans.items()}
    out = {'reference': analyse(b['ref']), 'seed_34': analyse(b['34'])}
    ws, agg = [], {n: {'G1': [], 'G2': 0, 'n': 0, 'G3': []} for n in ROLES}
    for s in range(1, 65):
        x = analyse(b[str(s)])
        ws.append(x['windows_share'])
        for n in ROLES:
            agg[n]['G1'].append(x[n]['G1_same_rhythm_within_phrase'])
            agg[n]['G2'] += x[n]['G2_phrases_with_bar4_fill']
            agg[n]['n'] += x[n]['phrases_playing_all_bars']
            agg[n]['G3'].append(x[n]['G3_phrase_rhythm_changes'])
    out['seeds_1_64'] = {'G6_windows_median': float(np.median(ws)), 'G6_min_max': [min(ws), max(ws)],
                         'per_role': {n: {'G1_median': float(np.median(v['G1'])), 'G2_fills': v['G2'], 'G2_phrases': v['n'],
                                          'G3_median': float(np.median(v['G3']))} for n, v in agg.items()}}
    out['expert'] = expert(Path(a.midi))
    out['expert_post_hoc_lags'] = expert_lags(Path(a.midi))
    ch = out['seed_34']['change_bars']
    g7 = []
    for t in DROPS:
        s = max(i for i in range(3) if t >= START[i])
        bar = s * 32 + int((t - START[s]) / BAR[s])
        prev = max([c for c in ch if c <= bar], default=0)
        g7.append({'t_s': t, 'bar': bar, 'bars_since_change': bar - prev})
    out['G7'] = {'drops': g7, 'median': float(np.median([x['bars_since_change'] for x in g7]))}
    w = windows(out['reference']['change_bars'])
    out['G8'] = {'covered_windows': sum(w), 'needed_for_9_of_12': max(0, 9 - sum(w))}
    print(json.dumps(out, indent=1))


if __name__ == '__main__':
    main()
