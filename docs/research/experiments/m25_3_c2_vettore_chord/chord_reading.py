# Vettore against the chord inferred from the Corpo bass (diatonic triad of the section mode):
# chord tones on beat 1 of chord changes, and Vettore notes forming interval class 1 or 6 with a Corpo voice.
# Usage: python chord_reading.py <intents.csv>
import csv, math, sys
from collections import defaultdict, Counter
rows = [r for r in csv.reader(open(sys.argv[1])) if r and r[0] != 'event_id' and not r[0].startswith('#')]
m = lambda hz: round(69 + 12 * math.log2(float(hz) / 440))
corpo = defaultdict(list)
for r in rows:
    if r[3] == '1': corpo[int(r[4])].append((m(r[6]), int(r[5]), int(r[2])))
frames = sorted(corpo)
scales = {0: [2, 4, 5, 7, 9, 11, 0], 1: [7, 9, 10, 0, 2, 3, 5], 2: [2, 4, 5, 7, 9, 11, 0]}
ic = lambda a, b: min((a - b) % 12, (b - a) % 12)
prev = None; st = Counter()
for i, f in enumerate(frames):
    notes = sorted(corpo[f]); bass = notes[0][0]; ph = notes[0][2]; sec = 0 if ph < 8 else 1 if ph < 16 else 2
    end = f + notes[0][1] if notes[0][1] > 0 else (frames[i + 1] if i + 1 < len(frames) else f + 96000)
    sc = scales[sec]; k = sc.index(bass % 12); triad = {sc[k], sc[(k + 2) % 7], sc[(k + 4) % 7]}
    change = prev is None or prev != bass % 12
    for x in rows:
        if x[3] != '3' or not (f <= int(x[4]) < end): continue
        v = m(x[6])
        if int(x[4]) == f and change:
            st[(sec, 'beat1_change')] += 1
            st[(sec, 'beat1_change_chord_tone')] += (v % 12) in triad
        if any(ic(v, c[0]) in (1, 6) for c in notes): st[(sec, 'clash_with_corpo')] += 1
    prev = bass % 12
for k, v in sorted(st.items()): print(k, v)
