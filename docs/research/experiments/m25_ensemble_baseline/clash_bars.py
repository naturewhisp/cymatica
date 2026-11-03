import csv, math, os, sys

root = sys.argv[1]
rows = []
for r in csv.DictReader(open(os.path.join(root, 'docs', 'research', 'experiments', 'cacp_methods', 'fixture_intents_seed1337.csv'), encoding='utf-8')):
    try:
        rows.append({k: (float(v) if k in ('pitch_hz', 'velocity') else int(v)) for k, v in r.items()})
    except (TypeError, ValueError):
        pass
rows = [r for r in rows if r['section'] in (0, 1, 2)]
N = ['C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G', 'Ab', 'A', 'Bb', 'B']
pc = lambda hz: N[round(69 + 12 * math.log2(hz / 440.0)) % 12]
starts = {0: 0, 1: 3072000, 2: 5705142}
bar = {0: 96000.0, 1: 96000.0 * 120 / 140, 2: 96000.0}
for sec, ph in ((0, 1), (1, 1), (2, 1)):
    print('section', sec, 'phrase', ph)
    for b in range(4):
        o = starts[sec] + (ph * 4 + b) * bar[sec]
        e = o + bar[sec]
        inbar = [r for r in rows if r['section'] == sec and o - 1 <= r['sample_frame'] < e - 1]
        corpo = [pc(r['pitch_hz']) for r in inbar if r['role'] == 1]
        vett = [pc(r['pitch_hz']) for r in sorted(inbar, key=lambda x: x['sample_frame']) if r['role'] == 3]
        print(f'  bar {b}: Corpo {corpo}  Vettore {vett}')
