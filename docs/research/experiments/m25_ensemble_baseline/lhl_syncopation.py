import math, csv, collections
W=[0]*16
for i in range(16):
    W[i]=0 if i==0 else -1 if i==8 else -2 if i%8==4 else -3 if i%4==2 else -4
def lhl(on,n=16):
    on=sorted(set(on));s=0;det=[]
    for k,i in enumerate(on):
        j=on[(k+1)%len(on)]
        gap=[(i+d)%n for d in range(1,((j-i)%n or n))]
        if not gap: continue
        R=max(W[g] for g in gap)
        if R>W[i]: s+=R-W[i];det.append((i,R-W[i]))
    return s,det
def e0(on,period=4,n=16):
    return {ph:sum(1 for t in range(ph,n,period) if t not in on) for ph in range(period)}
P={'K_A1 2,5,7,10,13,15':[2,5,7,10,13,15],'K_A2 0,4,8,12':[0,4,8,12],'K_B/A2 2,6,10,14':[2,6,10,14],
'expert dritto 0,3,5,8,11,13':[0,3,5,8,11,13],'expert storto 1,4,6,9,12,15':[1,4,6,9,12,15],
'Trama A 2,5,7,9,14':[2,5,7,9,14],'Trama B 0,3,5,7,12':[0,3,5,7,12],'Vettore 0,4,8,12':[0,4,8,12],
'K_A1+backbeat(4,12)':[2,5,7,10,13,15,4,12],'K_B+backbeat(4,12)':[2,6,10,14,4,12],'dritto+backbeat':[0,3,5,8,11,13,4,12],
'K_A1 + kick on 0':[0,2,5,7,10,13,15],'K_B + kick on 0':[0,2,6,10,14]}
for k,v in P.items():
    print(k,'LHL',lhl(v)[0],'E0(q)',e0(set(v)),'E0(half)',e0(set(v),8))
