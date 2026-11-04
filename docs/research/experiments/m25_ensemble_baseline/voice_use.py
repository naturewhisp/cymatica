"""Voices actually used per role and lowest notes, from the fixture intents (seed 1337).
Durations are the planned ones: release tails and open pads (duration 0) are ignored."""
import csv, sys

rows = []
for r in csv.DictReader(open(sys.argv[1], encoding='utf-8')):
    try:
        rows.append({k: (float(v) if k in ('pitch_hz', 'velocity') else int(v)) for k, v in r.items()})
    except (TypeError, ValueError):
        pass
rows = [r for r in rows if r['section'] in (0, 1, 2)]
names = ['Pulso', 'Corpo', 'Trama', 'Vettore']
for role in range(4):
    points = []
    durations = []
    for r in rows:
        if r['role'] == role and r['duration_frames'] > 0:
            points += [(r['sample_frame'], 1), (r['sample_frame'] + r['duration_frames'], -1)]
            durations.append(r['duration_frames'] / 48.0)
    points.sort(key=lambda p: (p[0], p[1]))
    count = peak = 0
    for _, delta in points:
        count += delta
        peak = max(peak, count)
    print(f'{names[role]}: max voices {peak}, note duration {min(durations):.0f}-{max(durations):.0f} ms')
for section in (0, 1, 2):
    corpo = min(r['pitch_hz'] for r in rows if r['role'] == 1 and r['section'] == section)
    pulso = min(r['pitch_hz'] for r in rows if r['role'] == 0 and r['section'] == section)
    print(f'section {section}: lowest Corpo {corpo:.1f} Hz, Pulso {pulso:.1f} Hz')
