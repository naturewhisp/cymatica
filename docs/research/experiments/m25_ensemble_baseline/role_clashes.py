import csv, math, os, sys
from collections import defaultdict

root = sys.argv[1]
rows = []
for r in csv.DictReader(open(os.path.join(root, 'docs', 'research', 'experiments', 'cacp_methods', 'fixture_intents_seed1337.csv'), encoding='utf-8')):
    try:
        rows.append({k: (float(v) if k in ('pitch_hz', 'velocity') else int(v)) for k, v in r.items()})
    except (TypeError, ValueError):
        pass
rows = [r for r in rows if r['section'] in (0, 1, 2)]
print('note-ons', len(rows), 'duration 0:', sum(1 for r in rows if r['duration_frames'] == 0))

def midi(hz):
    return round(69 + 12 * math.log2(hz / 440.0))

NAMES = ['C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G', 'Ab', 'A', 'Bb', 'B']
names = {0: 'Pulso', 1: 'Corpo', 2: 'Trama', 3: 'Vettore'}
ends_open = {}
# open pads: extend to the next Corpo onset
corpo = sorted({r['sample_frame'] for r in rows if r['role'] == 1})
notes = []
for r in rows:
    d = r['duration_frames']
    if d == 0:
        nxt = next((f for f in corpo if f > r['sample_frame']), r['sample_frame'] + 96000)
        d = nxt - r['sample_frame']
    notes.append(dict(role=r['role'], start=r['sample_frame'], end=r['sample_frame'] + d, m=midi(r['pitch_hz']), sec=r['section'], ph=r['phrase']))

events = sorted({n['start'] for n in notes} | {n['end'] for n in notes})
overlap = defaultdict(float)     # (roleA, roleB, section) -> ic -> frames
total_sec = defaultdict(float)
worst = defaultdict(float)
import bisect
active = []
idx_start = sorted(notes, key=lambda n: n['start'])
pos = 0
for i in range(len(events) - 1):
    a, b = events[i], events[i + 1]
    dur = b - a
    while pos < len(idx_start) and idx_start[pos]['start'] <= a:
        active.append(idx_start[pos])
        pos += 1
    active = [n for n in active if n['end'] > a]
    for x in range(len(active)):
        for y in range(x + 1, len(active)):
            n1, n2 = active[x], active[y]
            if n1['role'] == n2['role'] or 0 in (n1['role'], n2['role']):
                continue
            ra, rb = sorted((n1['role'], n2['role']))
            ic = abs(n1['m'] - n2['m']) % 12
            ic = min(ic, 12 - ic)
            sec = n1['sec'] if n1['start'] >= n2['start'] else n2['sec']
            overlap[(ra, rb, sec)] = overlap.get((ra, rb, sec), None) or defaultdict(float)
            overlap[(ra, rb, sec)][ic] += dur
            if ic in (1, 6):
                worst[(ra, rb, ic, n1['m'] % 12, n2['m'] % 12)] += dur

print('share of overlap time per interval class (ic1 = semitone/maj7, ic6 = tritone)')
for key in sorted(overlap):
    ra, rb, sec = key
    d = overlap[key]
    tot = sum(d.values())
    print(f'{names[ra]:>7}-{names[rb]:<7} sec{sec}: ' + ' '.join(f'ic{ic}={100 * d[ic] / tot:4.1f}%' for ic in range(7)))

print('largest ic1/ic6 pairs (seconds)')
for k, v in sorted(worst.items(), key=lambda kv: -kv[1])[:10]:
    ra, rb, ic, p1, p2 = k
    print(f'  {names[ra]}-{names[rb]} ic{ic} {NAMES[p1]}/{NAMES[p2]}: {v / 48000:.1f} s')

print('register by role and section (midi min..max)')
for sec in (0, 1, 2):
    print(sec, {names[r]: (min(n['m'] for n in notes if n['role'] == r and n['sec'] == sec), max(n['m'] for n in notes if n['role'] == r and n['sec'] == sec)) for r in range(4) if any(n['role'] == r and n['sec'] == sec for n in notes)})

# Vettore vs bar chord tones sounded by Corpo: same pitch class as one of the two Corpo notes at onset
hit = defaultdict(lambda: [0, 0])
for n in notes:
    if n['role'] != 3:
        continue
    pcs = {c['m'] % 12 for c in notes if c['role'] == 1 and c['start'] <= n['start'] < c['end']}
    hit[n['sec']][0] += (n['m'] % 12) in pcs
    hit[n['sec']][1] += 1
print('Vettore onsets sharing a pitch class with Corpo:', {k: f'{v[0]}/{v[1]}' for k, v in hit.items()})
