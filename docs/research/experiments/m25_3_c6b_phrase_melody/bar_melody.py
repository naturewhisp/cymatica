# T2: Vettore pitch sequences per bar inside each phrase; phrases with at least 3 distinct bars, and the phrase
# melody of the first phrase of each section (note names). Usage: python bar_melody.py <intents.csv>
import csv, math, sys
from collections import defaultdict
rows = [r for r in csv.reader(open(sys.argv[1])) if r and r[0] != 'event_id' and not r[0].startswith('#')]
m = lambda hz: round(69 + 12 * math.log2(float(hz) / 440))
N = ['C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G', 'Ab', 'A', 'Bb', 'B']
start = {}
for r in rows: start[int(r[2])] = min(start.get(int(r[2]), 10**12), int(r[4]))
start[24] = start[23] + (start[23] - start[22])
count = 0
for p in range(24):
    bar = (start[p + 1] - start[p]) / 4
    bars = defaultdict(list)
    for r in sorted((r for r in rows if r[3] == '3' and int(r[2]) == p), key=lambda r: int(r[4])):
        bars[min(3, int((int(r[4]) - start[p] + 1) // bar))].append(m(r[6]))
    distinct = len({tuple(v) for v in bars.values()})
    count += distinct >= 3
    if p in (0, 8, 16):
        print('phrase', p, ' | '.join(' '.join(N[x % 12] for x in bars[b]) for b in range(4)))
print('phrases with >= 3 distinct bars:', count, 'of 24')
