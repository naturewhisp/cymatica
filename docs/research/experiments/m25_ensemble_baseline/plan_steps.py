import csv, os, sys

root = sys.argv[1]
rows = []
for r in csv.DictReader(open(os.path.join(root, 'docs', 'research', 'experiments', 'cacp_methods', 'fixture_intents_seed1337.csv'), encoding='utf-8')):
    try:
        rows.append({k: (float(v) if k in ('pitch_hz', 'velocity') else int(v)) for k, v in r.items()})
    except (TypeError, ValueError):
        pass
rows = [r for r in rows if r['section'] in (0, 1, 2)]
print('rows', len(rows))
starts = {0: 0, 1: 3072000, 2: 5705142}
bar = {0: 96000.0, 1: 96000.0 * 120 / 140, 2: 96000.0}
names = {0: 'Pulso', 1: 'Corpo', 2: 'Trama', 3: 'Vettore'}
for role in range(4):
    print(names[role])
    for sec in (0, 1, 2):
        out = []
        for ph in range(8):
            s = set()
            for r in rows:
                if r['section'] == sec and r['phrase'] == ph and r['role'] == role:
                    rel = (r['sample_frame'] - starts[sec]) % bar[sec]
                    s.add(round(rel / (bar[sec] / 16)) % 16)
            out.append(','.join(map(str, sorted(s))) or '-')
        print(f'  sec{sec}: ' + ' | '.join(out))
