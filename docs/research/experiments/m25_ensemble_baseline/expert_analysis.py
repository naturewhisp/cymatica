import csv, hashlib, json, math, os, re, sys, wave
import numpy as np

root = sys.argv[1]
pkg = os.path.join(root, 'artifacts', 'expert_listening', 'cymatica_ascolto_esperto_2026-11-03')
key_path = os.path.join(root, 'artifacts', 'expert_listening', 'CHIAVE_clip_NON_INVIARE.txt')

def sha(p):
    h = hashlib.sha256()
    with open(p, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()

def load(p):
    with wave.open(p, 'rb') as w:
        assert w.getsampwidth() == 2
        x = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').astype(np.float64) / 32768.0
        return x.reshape(-1, w.getnchannels())

def rms(x):
    return float(np.sqrt(np.mean(x ** 2)))

def db(v):
    return 20 * math.log10(v) if v > 0 else float('-inf')

print('== package integrity')
a = sha(os.path.join(pkg, '01_brano_completo', 'brano_completo_A-B-A.wav'))
b = sha(os.path.join(root, 'artifacts', 'cacp', 'procedural_v1', 'procedural_v1.wav'))
print('brano sha', a[:16], 'procedural_v1 sha', b[:16], 'identical', a == b)

key = {}
for line in open(key_path, encoding='utf-8'):
    m = re.match(r'(frase_[AB])/(clip_\d)\.wav\s+=\s+\S+\s+rilascio (\d+) ms.*sha256 ([0-9a-f]{64})', line)
    if m:
        key[(m.group(1), m.group(2))] = (int(m.group(3)), m.group(4))
bad = 0
for (ph, cl), (ms, h) in key.items():
    if sha(os.path.join(pkg, '03_clip_ab', ph, cl + '.wav')) != h:
        bad += 1
print('clips', len(key), 'sha mismatches', bad)

print('== null tests')
for ph in ('frase_A', 'frase_B'):
    clips = {}
    for i in range(1, 6):
        cl = f'clip_{i}'
        clips[i] = (key[(ph, cl)][0], load(os.path.join(pkg, '03_clip_ab', ph, cl + '.wav')))
    order = [clips[i][0] for i in range(1, 6)]
    print(ph, 'presented order (ms):', order)
    ref = next(x for ms, x in clips.values() if ms == 2)
    for i in range(1, 6):
        ms, x = clips[i]
        print(f'  clip_{i} {ms:>2} ms: rms {db(rms(x)):.2f} dBFS peak {db(float(np.max(np.abs(x)))):.2f} dBFS, diff from 2 ms vs mix {db(rms(x - ref)) - db(rms(ref)) if ms != 2 else float("nan"):.2f} dB')
    for i in range(1, 5):
        m1, x1 = clips[i]
        m2, x2 = clips[i + 1]
        print(f'  adjacent {m1}->{m2} ms: diff vs mix {db(rms(x2 - x1)) - db(rms(x1)):.2f} dB')

print('== plan at the section boundaries')
rows = []
for r in csv.DictReader(open(os.path.join(root, 'docs', 'research', 'experiments', 'cacp_methods', 'fixture_intents_seed1337.csv'), encoding='utf-8')):
    try:
        rows.append({k: (float(v) if k in ('pitch_hz', 'velocity') else int(v)) for k, v in r.items()})
    except (TypeError, ValueError):
        pass
rows = [r for r in rows if r['section'] in (0, 1, 2)]

def bar_frames(section):
    return 96000.0 if section in (0, 2) else 96000.0 * 120 / 140

starts = {0: 0, 1: 3072000, 2: 5705142}
def steps(section, phrase, role):
    bf = bar_frames(section)
    s = set()
    for r in rows:
        if r['section'] == section and r['phrase'] == phrase and r['role'] == role:
            rel = (r['sample_frame'] - starts[section]) % bf
            s.add(round(rel / (bf / 16)) % 16)
    return sorted(s)

names = {0: 'Pulso', 1: 'Corpo', 2: 'Trama', 3: 'Vettore'}
for label, sec, ph in [('A last', 0, 7), ('B first', 1, 0), ('B last', 1, 7), ('A\' first', 2, 0)]:
    print(label)
    for role in range(4):
        ev = [r for r in rows if r['section'] == sec and r['phrase'] == ph and r['role'] == role]
        hz = sorted({round(r['pitch_hz'], 2) for r in ev})
        print(f'  {names[role]}: events {len(ev)}, steps {steps(sec, ph, role)}, vel {sorted({round(r["velocity"], 2) for r in ev})}, pitches {hz[:8]}')

print('== presence per phrase')
m = json.load(open(os.path.join(root, 'artifacts', 'cacp', 'procedural_v1_manifest.json'), encoding='utf-8'))
pp = m['presence_per_phrase']
print(type(pp), (list(pp.keys())[:10] if isinstance(pp, dict) else len(pp)))
sample = pp if isinstance(pp, dict) else pp[0]
print(json.dumps(sample, indent=0)[:900])
