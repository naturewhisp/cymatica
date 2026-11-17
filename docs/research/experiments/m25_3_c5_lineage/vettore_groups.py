# Vettore groups per phrase by identical interval and inter-onset sequences, onsets quantized to the sixteenth grid
# of each section like fixturePhraseTokens (equivalent to M5r distance 0).
# Usage: python vettore_groups.py <intents.csv>
import csv, math, sys
m = lambda hz: round(69 + 12 * math.log2(float(hz) / 440))
seq = {}
for r in csv.reader(open(sys.argv[1])):
    if r and r[0] != 'event_id' and not r[0].startswith('#') and r[3] == '3':
        step = 48000 * 60 / (140 if 8 <= int(r[2]) < 16 else 120) / 4
        seq.setdefault(int(r[2]), []).append((round(int(r[4]) / step), m(r[6])))
groups, labels = [], []
for p in range(24):
    v = sorted(seq[p]); key = (tuple(b[1] - a[1] for a, b in zip(v, v[1:])), tuple(b[0] - a[0] for a, b in zip(v, v[1:])))
    if key not in groups: groups.append(key)
    labels.append(groups.index(key))
print('groups', len(groups), 'labels', labels)
