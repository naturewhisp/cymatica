import math
def J(n,x,N=2000):
    s=0
    for k in range(N):
        t=(k+.5)*math.pi/N
        s+=math.cos(n*t-x*math.sin(t))
    return s/N
def fm(c,m,I,kmax=12):
    d={}
    def add(f,a):
        if f<0: f=-f;a=-a
        key=round(f,3)
        d[key]=d.get(key,0)+a
    add(c,J(0,I))
    for n in range(1,kmax):
        j=J(n,I)
        add(c+n*m,j)
        add(c-n*m,((-1)**n)*j)
    return [(f,abs(a)) for f,a in sorted(d.items()) if abs(a)>1e-3 and f>0]
def diss(sp):
    Dstar,S1,S2,C1,C2,A1,A2=.24,.0207,18.96,5,-5,-3.51,-5.75
    s=0
    sp=sorted(sp)
    for i in range(len(sp)):
        for j in range(i+1,len(sp)):
            f1,a1=sp[i];f2,a2=sp[j]
            S=Dstar/(S1*f1+S2);d=f2-f1
            s+=min(a1,a2)*(C1*math.exp(A1*S*d)+C2*math.exp(A2*S*d))
    return s
def saw(f,n=10):
    return [(f*k,1/k) for k in range(1,n+1)]
ratios=[('1:1',1),('1:2',2),('1:3',3),('2:3',1.5),('1:sqrt2',2**.5),('1:1.4',1.4),('1:1.76',1.76),('1:1.88',1.88),('1:2.76',2.76)]
print('self-dissonance single FM note, c=587.33 (D5) and 392 (G4), I=0.5,1.5,3')
for name,r in ratios:
    out=[]
    for c in (392.0,587.33,880.0):
        for I in (0.5,1.5,3.0):
            out.append(round(diss(fm(c,c*r,I)),3))
    print(name,out)
print('saw 10 harm 587', round(diss(saw(587.33)),3))
# spectrum listing 1:1.88 I=3
sp=fm(1,1.88,3.0)
print([(round(f,2),round(a,2)) for f,a in sp if a>.03])
sp=fm(1,1.88,1.5)
print([(round(f,2),round(a,2)) for f,a in sp if a>.03])
# Trama FM note against Corpo dyad (D3 + A4 saw) : increment of dissonance
base=saw(146.83)+saw(440.0)
b0=diss(base)
print('base dyad',round(b0,3))
for pname,c in (('D5',587.33),('F5',698.46),('A5',880.0),('C6',1046.5),('E5',659.26)):
    row=[]
    for name,r in ratios:
        s=fm(c,c*r,1.5)
        d=diss(base+s)-b0-diss(s)
        row.append((name,round(d,2)))
    print(pname,row)
# dissonance curve self-interval
def curve(sp_f):
    out=[]
    r=1.0
    while r<=2.02:
        out.append((r,diss(sp_f(1.0)+sp_f(r))))
        r*=2**(1/120)
    return out
def minima(c,thr=0.0):
    m=[]
    for i in range(1,len(c)-1):
        if c[i][1]<c[i-1][1] and c[i][1]<c[i+1][1]:
            m.append((round(1200*math.log2(c[i][0])),round(c[i][1],2)))
    return m
for name,r in ratios:
    f=lambda k:fm(440*k,440*k*r,1.5)
    c=curve(f)
    print(name,'minima cents',minima(c),'P5',round(diss(f(1)+f(2**(7/12))),2),'M3',round(diss(f(1)+f(2**(4/12))),2),'m2',round(diss(f(1)+f(2**(1/12))),2),'TT',round(diss(f(1)+f(2**.5)),2),'P8',round(diss(f(1)+f(2)),2))
f=lambda k:saw(440*k,8)
c=curve(f);print('saw8 minima',minima(c))
