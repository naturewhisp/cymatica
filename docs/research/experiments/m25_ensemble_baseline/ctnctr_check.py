import csv, math, sys, collections
rows=[]
for r in csv.DictReader(open(sys.argv[1],encoding='utf-8')):
    try: rows.append(dict(sec=int(r['section']),role=int(r['role']),f=int(r['sample_frame']),hz=float(r['pitch_hz'])))
    except (TypeError,ValueError,KeyError): pass
starts={0:0,1:3072000,2:5705142}; barlen={0:96000.0,1:96000.0*120/140,2:96000.0}
midi=lambda hz:int(round(69+12*math.log2(hz/440.0)))
A=[{2,5,9},{7,11,2},{9,0,4},{2,5,9}]; B=[{7,10,2},{3,7,10},{5,9,0},{7,10,2}]
ch={0:A,1:B,2:A}
def score(iv):
    iv%=12
    if iv in (0,3,4,7,8,9): return 1
    if iv==5: return 0
    return -1
for s in (0,1,2):
    v=sorted([r for r in rows if r['sec']==s and r['role']==3],key=lambda x:x['f'])
    data=[]
    for i,r in enumerate(v):
        pos=(r['f']-starts[s])/barlen[s]; bar=int(math.floor(pos+1e-3)); beat=int(round(max(0,pos-bar)*4))
        m=midi(r['hz']); c=ch[s][bar%4]
        nxt=midi(v[i+1]['hz']) if i+1<len(v) else None
        ct=(m%12) in c
        np=(not ct) and nxt is not None and abs(nxt-m)<=2
        pcs=sum(score(m-x) for x in c)
        data.append((beat,ct,np,pcs))
    def agg(sel):
        d=[x for x in data if sel(x)]
        nc=sum(1 for x in d if x[1]); nn=sum(1 for x in d if not x[1]); np=sum(1 for x in d if x[2])
        return dict(n=len(d),ct_rate=round(nc/len(d),3),ctnctr=round((nc+np)/(nc+nn),3),pcs=round(sum(x[3] for x in d)/len(d),3))
    print('sec',s,'all',agg(lambda x:True))
    print('   beats1,3',agg(lambda x:x[0] in (0,2)))
    print('   beats2,4',agg(lambda x:x[0] in (1,3)))
