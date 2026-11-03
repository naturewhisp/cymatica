# Clock formulas of MusicClock (dossier A §1): fixture boundaries, beat lengths at 140 BPM and the exact inversion
# e(dF) = q + [r >= N - (D - 1)] checked against brute force on random tempo maps. Standard library only.
import random

FS = 48000


def k(bpm_num, bpm_den, ts_den):
    return 60 * FS * bpm_den * 4, bpm_num * ts_den


def frames(beats, kk):
    return (beats * kk[0]) // kk[1]


ka, kb = k(120, 1, 4), k(140, 1, 4)
start_b = frames(128, ka)
start_a2 = start_b + frames(128, kb)
end = start_a2 + frames(128, ka)
print('start B', start_b, 'start A2', start_a2, 'noteOff A->B', start_b + frames(1, kb), 'noteOff B->A2', start_a2 + frames(1, ka))
print('end', end, end / FS, 's')
print('B beat lengths', [frames(i + 1, kb) - frames(i, kb) for i in range(7)])

random.seed(1)
bad = 0
for _ in range(200000):
    num, den, ts = random.randint(30, 300), random.randint(1, 7), random.choice([2, 4, 8, 16])
    n, d = 60 * FS * den * 4, num * ts
    if d > n:
        continue
    df = random.randint(0, 10**9)
    q, r = divmod(df * d, n)
    b = q + (1 if r >= n - (d - 1) else 0)
    bad += not ((b * n) // d <= df and ((b + 1) * n) // d > df)
print('inversion mismatches', bad)
