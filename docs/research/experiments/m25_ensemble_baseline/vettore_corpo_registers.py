import csv,math,collections,sys
rows=[r for r in csv.DictReader(open(sys.argv[1])) if r['sample_frame'] is not None and not r['event_id'].startswith('#')]
for r in rows:
    r['role']=int(r['role']);r['s']=int(r['section']);r['f']=int(r['sample_frame']);r['d']=int(r['duration_frames']);r['hz']=float(r['pitch_hz']);r['m']=round(69+12*math.log2(r['hz']/440));r['v']=float(r['velocity'])
start={s:min(r['f'] for r in rows if r['s']==s) for s in (0,1,2)}
bar={0:96000.0,1:(start[2]-start[1])/32,2:96000.0}
def pos(r):
    b=(r['f']-start[r['s']])/bar[r['s']];B=int(b+1e-6);return B,(b-B)*16
NN='C C# D Eb E F F# G Ab A Bb B'.split()
# Corpo per bar: root=lowest, chord triad by mode
corp=collections.defaultdict(list)
for r in rows:
    if r['role']==1: corp[(r['s'],pos(r)[0])].append(r)
scale={0:[2,4,5,7,9,11,0],1:[7,9,10,0,2,3,5],2:[2,4,5,7,9,11,0]}
def triad(sec,rootpc):
    sc=scale[sec];i=sc.index(rootpc);return {sc[i],sc[(i+2)%7],sc[(i+4)%7]}
def snd(role,t):
    return [r for r in rows if r['role']==role and r['f']<=t<r['f']+r['d']]
ROOT=collections.Counter()
stats=collections.defaultdict(lambda:collections.Counter())
pcs=collections.defaultdict(list)
vet=[r for r in rows if r['role']==3];vet.sort(key=lambda r:r['f'])
seqbar=collections.defaultdict(list)
for i,r in enumerate(vet):
    B,st=pos(r);sec=r['s']
    cs=snd(1,r['f'])
    if not cs: stats[sec]['noCorpo']+=1;continue
    root=min(c['m'] for c in cs)%12
    if root not in scale[sec]: stats[sec]['rootOOS']+=1;continue
    ch=triad(sec,root)
    pc=r['m']%12
    next_m=vet[i+1]['m'] if i+1<len(vet) else r['m']
    strong='beat13' if st in (0.0,8.0) else 'beat24'
    st_key=(sec,strong)
    if pc in ch: stats[sec]['nc']+=1;stats[st_key]['nc']+=1
    else:
        stats[sec]['nn']+=1;stats[st_key]['nn']+=1
        if abs(next_m-r['m'])<=2: stats[sec]['np']+=1;stats[st_key]['np']+=1
    # PCS: sum over triad
    sc=0
    for c in ch:
        iv=(pc-c)%12
        sc+= 1 if iv in (0,3,4,7,8,9) else 0 if iv==5 else -1
    stats[sec]['pcs']+=sc;stats[sec]['pcsn']+=1;stats[st_key]['pcs']+=sc;stats[st_key]['pcsn']+=1
    # register relation
    hi=max(c['m'] for c in cs);lo=min(c['m'] for c in cs)
    stats[sec]['above_upper']+= r['m']>hi
    stats[sec]['below_upper']+= r['m']<hi
    stats[sec]['eq_upper']+= r['m']==hi
    stats[sec]['eq_any']+= r['m'] in [c['m'] for c in cs]
    stats[sec]['dist_upper_sum']+=abs(r['m']-hi)
    stats[sec]['le2st']+= min(abs(r['m']-c['m']) for c in cs)<=2 and r['m'] not in [c['m'] for c in cs]
    seqbar[(sec,B)].append(NN[pc])
for k in sorted(stats,key=str):
    s=stats[k]
    if s.get('nc',0)+s.get('nn',0):
        ct=(s['nc']+s['np'])/(s['nc']+s['nn'])
        print(k,'CTnCTR',round(ct,3),'CTrate',round(s['nc']/(s['nc']+s['nn']),3),'PCS',round(s['pcs']/s['pcsn'],2),{a:b for a,b in s.items() if a in('above_upper','below_upper','eq_upper','eq_any','le2st','noCorpo')},'meanDistUpper',round(s['dist_upper_sum']/max(1,s['pcsn']),1))
# melodic ostinato: distinct 4-note sequences per section
for sec in (0,1,2):
    c=collections.Counter(tuple(v) for (s,b),v in seqbar.items() if s==sec)
    print(sec,c.most_common(5))
# chord sequence
print([ (k,sorted(NN[c['m']%12]+str(c['m']//12-1) for c in v)) for k,v in sorted(corp.items())][:8])
print([ (k,sorted(NN[c['m']%12]+str(c['m']//12-1) for c in v)) for k,v in sorted(corp.items()) if k[0]==1][:8])
# onset synchrony
for sec in (0,1,2):
    on=collections.defaultdict(set)
    for r in rows:
        if r['s']==sec: on[r['role']].add(r['f'])
    def sync(a,b): 
        return round(len(on[a]&on[b])/max(1,len(on[a])),2)
    print('sec',sec,'P&V/P',sync(0,3),'V&P/V',sync(3,0),'T&P/T',sync(2,0),'T&V/T',sync(2,3))
# by bar: active roles (with onset in bar)
act=collections.Counter()
for sec in (0,1,2):
    for B in range(32):
        n=set(r['role'] for r in rows if r['s']==sec and pos(r)[0]==B)
        act[(sec,len(n))]+=1
print(sorted(act.items()))
