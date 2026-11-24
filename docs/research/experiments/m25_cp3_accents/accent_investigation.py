"""Measures A1-A6 of docs/research/M2_5_CP3_I8_Accent_Investigation_Preregistration.md (aggregate statistics only).

  python -I accent_investigation.py --plan <csv> --midi <expert MIDI dir> --out <json>
"""
import argparse
import json
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
for q in (HERE.parent / 'm25_cp3_listening', HERE.parent / 'expert_production_analysis'):
    sys.path.insert(0, str(q))
import cp3_material as CM3  # noqa: E402
import midi_analysis as MA  # noqa: E402

T, BP = CM3.T, CM3.BP
W = [0, -4, -3, -4, -2, -4, -3, -4, -1, -4, -3, -4, -2, -4, -3, -4]  # LHL weights, 4/4 sixteenths


def lhl(on):
    """Sum over rests that end a note of (rest weight - note weight) when positive, cyclic bar."""
    on = sorted(on)
    s = 0
    for i, a in enumerate(on):
        b = on[(i + 1) % len(on)] + (16 if i == len(on) - 1 else 0)
        rests = [W[k % 16] for k in range(a + 1, b)]
        if rests:
            s += max(0, max(rests) - W[a])
    return s


def spearman(x, y):
    def rank(v):
        v = np.asarray(v, float)
        o = np.argsort(v, kind='stable')
        r = np.empty(len(v))
        r[o] = np.arange(len(v))
        for u in np.unique(v):
            r[v == u] = r[v == u].mean()
        return r
    a, b = rank(x), rank(y)
    return float(np.corrcoef(a, b)[0, 1]) if a.std() and b.std() else None


def metrics(bars):
    """bars: list of (kick set, snare set, bass set)."""
    k = sum(len(x[0]) for x in bars)
    hit = sum(len(x[0] & x[2]) for x in bars) / k
    chance = np.mean([sum(len(x[0] & {(p + s) % 16 for p in x[2]}) for x in bars) / k for s in range(1, 16)])
    n = sum(len(x[2]) for x in bars)
    joint = [x[0] | x[2] for x in bars]
    both = [sum(p in x[0] and p in x[2] for x in bars) / len(bars) for p in range(16)]
    return {'bars': len(bars), 'A1_kicks_with_bass': hit, 'A1_chance': float(chance), 'A1_excess': float(hit - chance),
            'A2_bass_on_kick_or_snare': sum(len(x[2] & (x[0] | x[1])) for x in bars) / n,
            'bass_on_kick': sum(len(x[2] & x[0]) for x in bars) / n,
            'A3_lhl_per_onset_joint': sum(lhl(j) for j in joint) / sum(len(j) for j in joint),
            'lhl_per_onset_kick': sum(lhl(x[0]) for x in bars) / k, 'lhl_per_onset_bass': sum(lhl(x[2]) for x in bars) / n,
            'A5_spearman': spearman([len(x[0]) + len(x[1]) for x in bars], [len(x[2]) for x in bars]),
            'A6_shared_positions': [p for p in range(16) if both[p] >= 0.5], 'both_share_by_position': both}


def expert(midi, bassfile):
    dr, bs = MA.read_smf(midi / 'Kick & Snare.mid'), MA.read_smf(midi / f'{bassfile}.mid')

    def pos(n, m):
        six = m['ppq'] / 4
        g = int(round(n['tick'] / six))
        return g // 16, g % 16
    K, S, B = {}, {}, {}
    for n in dr['notes']:
        b, p = pos(n, dr)
        (K if n['pitch'] == 36 else S).setdefault(b, set()).add(p)
    for n in bs['notes']:
        b, p = pos(n, bs)
        B.setdefault(b, set()).add(p)
    keep = [b for b in sorted(B) if K.get(b)]
    r = metrics([(K[b], S.get(b, set()), B[b]) for b in keep])
    v = {True: [], False: []}
    for n in bs['notes']:
        b, p = pos(n, bs)
        if b in K:
            v[p in K[b]].append(n['vel'])
    r['A4_velocity_on_kick_minus_others'] = float(np.median(v[True]) - np.median(v[False])) if v[True] and v[False] else None
    r['velocity_distinct_values'] = len(set(v[True] + v[False]))
    return r


def ours(ev):
    kicks = CM3.kicks_of(ev)
    bass, _ = BP.line(kicks)
    edges = [BP.bar_time(b) for b in range(96)]
    K, B = {}, {}
    for t in kicks:
        s = max(i for i in range(3) if t >= BP.START[i] - 1e-4)
        g = int(round((t - BP.START[s]) / (BP.BAR[s] / 16)))
        K.setdefault(s * 32 + g // 16, set()).add(g % 16)
    for e in bass:
        s = e['bar'] // 32
        g = int(round((e['t'] - BP.START[s]) / (BP.BAR[s] / 16)))
        B.setdefault(s * 32 + g // 16, set()).add(g % 16)
    del edges
    keep = [b for b in sorted(B) if K.get(b) and b // 4 not in BP.PEDAL | BP.SILENT]
    return metrics([(K[b], set(), B[b]) for b in keep])


def main():
    ap = argparse.ArgumentParser()
    for o in ('plan', 'midi', 'out'):
        ap.add_argument('--' + o, required=True)
    a = ap.parse_args()
    ev, m = T.read(a.plan), Path(a.midi)
    r = {'expert_808': expert(m, '808_Bass'), 'expert_sub': expert(m, 'SubBass'), 'today': ours(ev), 'Z': ours(CM3.z_plan(ev))}
    x, z = r['expert_808'], r['Z']
    r['outcome'] = {'A1': x['A1_excess'] >= 0.15 and z['A1_excess'] <= 0.08, 'A2': x['A2_bass_on_kick_or_snare'] >= 0.5,
                    'A3': z['A3_lhl_per_onset_joint'] > x['A3_lhl_per_onset_joint'],
                    'A4': (x['A4_velocity_on_kick_minus_others'] or 0) >= 5,
                    'A5': x['A5_spearman'] is not None and x['A5_spearman'] <= -0.2 and z['A5_spearman'] is not None and abs(z['A5_spearman']) <= 0.2,
                    'A6': len(x['A6_shared_positions']) >= 2 and len(z['A6_shared_positions']) <= 1}
    r['rule_three_of_A1_A3_A5_A6'] = sum(r['outcome'][k] for k in ('A1', 'A3', 'A5', 'A6')) >= 3
    Path(a.out).write_text(json.dumps(r, indent=1, default=lambda v: v.item()) + '\n', encoding='utf-8', newline='\n')
    for k, v in r.items():
        print(k, {q: (round(w, 3) if isinstance(w, float) else w) for q, w in v.items() if q != 'both_share_by_position'} if isinstance(v, dict) else v)


if __name__ == '__main__':
    main()
