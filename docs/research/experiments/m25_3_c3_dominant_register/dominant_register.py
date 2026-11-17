# Vettore against the Corpo upper voice sounding at each Vettore onset (pads held from the previous phrase included),
# Vettore range per section, and Vettore notes within 2 semitones of a Trama note sounding at the same time.
# Usage: python dominant_register.py <intents.csv>
import csv, math, sys
rows = [r for r in csv.reader(open(sys.argv[1])) if r and r[0] != 'event_id' and not r[0].startswith('#')]
m = lambda hz: round(69 + 12 * math.log2(float(hz) / 440))
notes = [dict(role=r[3], on=int(r[4]), off=int(r[4]) + (int(r[5]) if int(r[5]) > 0 else 96000), m=m(r[6]), ph=int(r[2])) for r in rows]
# open pads: end at the next Corpo onset after them
corpo_on = sorted({n['on'] for n in notes if n['role'] == '1'})
for n in notes:
    if n['role'] == '1' and n['off'] - n['on'] == 96000:
        n['off'] = next((f for f in corpo_on if f > n['on']), n['on'] + 96000)
for sec, phs in [('A', range(0, 8)), ('B', range(8, 16)), ("A'", range(16, 24))]:
    above = eq = below = near = tot = 0
    vs = [n for n in notes if n['role'] == '3' and n['ph'] in phs]
    for n in vs:
        tot += 1
        cu = [c['m'] for c in notes if c['role'] == '1' and c['on'] <= n['on'] < c['off']]
        if cu:
            u = max(cu); above += n['m'] > u; eq += n['m'] == u; below += n['m'] < u
        tr = [c['m'] for c in notes if c['role'] == '2' and c['on'] < n['off'] and c['off'] > n['on']]
        near += any(abs(n['m'] - t) <= 2 for t in tr)
    print(sec, 'range', min(n['m'] for n in vs), max(n['m'] for n in vs), 'above/eq/below', above, eq, below, 'of', tot, '| near Trama', near)
