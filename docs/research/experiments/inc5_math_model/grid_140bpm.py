# Phrase-relative step grid of section B (140 BPM 4/4) of fixture-m2-4-v1 (reconciliation §4.1, AG-MM-07): the
# frame-relative grids differ by +-1 sample from phrase to phrase, with period 7 phrases. Standard library only.
N, D = 60 * 48000 * 4, 140 * 4
START_B = 3072000


def frame(beats, step=0, steps_per_beat=4):
    return START_B + ((beats * steps_per_beat + step) * N) // (D * steps_per_beat)


grids = []
for p in range(8):
    b0 = p * 16
    start = frame(b0)
    grids.append(tuple(frame(b0 + b, s) - start for b in range(16) for s in range(4)))
print('distinct relative grids among the 8 B phrases:', len(set(grids)))
for p in range(1, 8):
    diff = [abs(a - b) for a, b in zip(grids[0], grids[p])]
    print('phrase', 8 + p, 'steps differing from phrase 8:', sum(1 for x in diff if x), 'max |d|', max(diff))
