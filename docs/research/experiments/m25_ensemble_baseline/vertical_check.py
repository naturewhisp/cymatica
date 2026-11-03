import csv, math, sys, collections
path = sys.argv[1]
rows = []
for r in csv.DictReader(open(path, encoding='utf-8')):
    try:
        rows.append(dict(event=int(r['event_id']), sec=int(r['section']), ph=int(r['phrase']), role=int(r['role']),
                         f=int(r['sample_frame']), d=int(r['duration_frames']), hz=float(r['pitch_hz']), v=float(r['velocity'])))
    except (TypeError, ValueError, KeyError):
        pass
print('rows', len(rows), 'roles', sorted({r['role'] for r in rows}), 'secs', sorted({r['sec'] for r in rows}))
N = ['C','C#','D','Eb','E','F','F#','G','Ab','A','Bb','B']
pc = lambda hz: round(69 + 12*math.log2(hz/440.0)) % 12
starts = {0:0, 1:3072000, 2:5705142}
barlen = {0:96000.0, 1:96000.0*120/140, 2:96000.0}
A = [{2,5,9},{7,11,2},{9,0,4},{2,5,9}]
B = [{7,10,2},{3,7,10},{5,9,0},{7,10,2}]
chords = {0:A, 1:B, 2:A}
names = {0:['Dm','G','Am','Dm'],1:['Gm','Eb','F','Gm'],2:['Dm','G','Am','Dm']}
vet = [r for r in rows if r['role']==3]
stats = collections.defaultdict(lambda: [0,0])
beat1chg = collections.defaultdict(lambda: [0,0])
seqs = collections.defaultdict(collections.Counter)
perbar = collections.defaultdict(list)
for r in sorted(vet, key=lambda x:x['f']):
    s = r['sec']; pos = (r['f']-starts[s])/barlen[s]
    bar = int(math.floor(pos+1e-3)); frac = max(0.0,pos-bar)
    beat = int(round(frac*4))
    b4 = bar % 4
    p = pc(r['hz'])
    ct = p in chords[s][b4]
    k = (s, 'odd' if beat in (0,2) else 'even')
    stats[k][0]+=ct; stats[k][1]+=1
    stats[(s,'all')][0]+=ct; stats[(s,'all')][1]+=1
    perbar[(s,bar)].append(N[p])
    if beat==0:
        k2=(s,names[s][b4]); beat1chg[k2][0]+=ct; beat1chg[k2][1]+=1
for k in sorted(stats, key=str):
    a,b=stats[k]; print(k, a, b, round(a/b,3))
print('--- beat 1 by chord (chord tone / total)')
for k in sorted(beat1chg, key=str):
    a,b=beat1chg[k]; print(k,a,b, round(a/b,3))
print('--- distinct per-bar ostinati')
for s in (0,1,2):
    c=collections.Counter(' '.join(v) for (sx,bar),v in perbar.items() if sx==s)
    print(s, c.most_common(6))
print('--- role-wise: which Vettore pattern per bar, per chord, with chord-tone flags (sec 1)')
out = collections.Counter()
for (s,bar),v in perbar.items():
    if s==1:
        out[(names[1][bar%4],' '.join(v))]+=1
for k,c in sorted(out.items()): print(k,c)
print('--- strong-beat rule (tempo 1) on chord-change bars: bars where chord differs from previous bar')
viol = collections.Counter(); tot=collections.Counter()
for s in (0,1,2):
    prev=None
    for bar in range(32):
        b4=bar%4
        ch=names[s][b4]
        first=perbar[(s,bar)][0]
        if prev is None or ch!=prev:
            tot[s]+=1
            if N.index(first) not in chords[s][b4]:
                viol[s]+=1
        prev=ch
print('chord-change downbeats total',dict(tot),'Vettore non-chord tone',dict(viol))
