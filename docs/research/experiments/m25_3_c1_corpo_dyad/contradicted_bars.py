# Counts the Corpo bars whose upper voice forms interval class 1 or 6 with a Vettore note attacking in the bar.
# Usage: python contradicted_bars.py <intents.csv>
import csv, math, sys
from collections import Counter, defaultdict
rows = [r for r in csv.reader(open(sys.argv[1])) if r and r[0] != 'event_id' and not r[0].startswith('#')]
m = lambda hz: round(69 + 12 * math.log2(float(hz) / 440))
corpo = defaultdict(list)
for r in rows:
    if r[3] == '1': corpo[int(r[4])].append((m(r[6]), int(r[5]), int(r[2])))
frames = sorted(corpo)
ic = lambda a, b: min((a - b) % 12, (b - a) % 12)
out = Counter(); tot = Counter(); kinds = Counter()
NAMES = ['C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G', 'Ab', 'A', 'Bb', 'B']
for i, f in enumerate(frames):
    notes = sorted(corpo[f]); bass, harm, ph = notes[0][0], notes[-1][0], notes[0][2]
    end = f + notes[0][1] if notes[0][1] > 0 else (frames[i + 1] if i + 1 < len(frames) else f + 96000)
    sec = 0 if ph < 8 else 1 if ph < 16 else 2
    tot[sec] += 1
    bad = sorted({m(r[6]) % 12 for r in rows if r[3] == '3' and f <= int(r[4]) < end and ic(m(r[6]), harm) in (1, 6)})
    if bad:
        out[sec] += 1
        kinds[(sec, NAMES[bass % 12], NAMES[harm % 12], ','.join(NAMES[b] for b in bad))] += 1
print('bars per section', dict(tot))
print('contradicted bars per section', dict(out))
for k, v in sorted(kinds.items()): print('  section %d bass %s upper %s vs Vettore %s: %d bars' % (k + (v,)))
