# CYMATICA M2.4 Inc-5: numeric checks for the clustered-claims dossier. Pure Python, no deps.
import math
Z=1.959963984540054
def wilson(x,n,z=Z):
    if n==0: return (None,None)
    p=x/n; d=1+z*z/n; c=(p+z*z/(2*n))/d; h=z*math.sqrt(p*(1-p)/n+z*z/(4*n*n))/d
    return (max(0,c-h),min(1,c+h))
def binom_pmf(k,n,p): return math.comb(n,k)*p**k*(1-p)**(n-k)
def cp_lower(x,n,a=0.025):
    if x==0: return 0.0
    lo,hi=0.0,1.0
    for _ in range(200):
        m=(lo+hi)/2
        tail=sum(binom_pmf(k,n,m) for k in range(x,n+1))  # P(X>=x|m)
        if tail<a: lo=m
        else: hi=m
    return (lo+hi)/2
print("== A. Lower bounds for k/k (all correct)")
print(" n  Wilson95low  n/(n+z2)  CP97.5%low  CP95%low(1-sided) ruleof3 upper err 3/n  exact 1-0.05^(1/n)")
for n in [1,2,3,4,5,6,8,9,10,12,15,16,20,30,33,35,36,40,74]:
    w=wilson(n,n)[0]
    print(f"{n:3d}  {w:.4f}      {n/(n+Z*Z):.4f}   {0.025**(1/n):.4f}     {0.05**(1/n):.4f}   {min(1,3/n):.4f}  {1-0.05**(1/n):.4f}")
print("thresholds L: min n with Wilson n/n low > L ; CP97.5 ; CP95")
for L in [0.5,0.7,0.8,0.9,0.95]:
    nw=next(n for n in range(1,1000) if n/(n+Z*Z)>L)
    nc=next(n for n in range(1,1000) if 0.025**(1/n)>L)
    nc5=next(n for n in range(1,1000) if 0.05**(1/n)>L)
    print(f" L={L}: Wilson n>={nw}, CP97.5 n>={nc}, CP95 n>={nc5}")

def mover_J_lower(x1,n1,x2,n2):
    s=x1/n1; c=x2/n2; l1=wilson(x1,n1)[0]; l2=wilson(x2,n2)[0]
    return s+c-1-math.sqrt((s-l1)**2+(c-l2)**2)
def sum_rule(x1,n1,x2,n2): return wilson(x1,n1)[0]+wilson(x2,n2)[0]-1
print("\n== B. Worked examples: J lower bounds")
cases=[("per-claim Antigravity 36/36, 4/4",36,36,4,4),
       ("window unit 9/9, 4/4",9,9,4,4),
       ("current checker per-claim 33/33, 3/3",33,33,3,3),
       ("window unit 9/9, 3/3",9,9,3,3),
       ("8 windows 8/8, 3/3",8,8,3,3),
       ("per-claim 36/36, 1/1",36,36,1,1),
       ("window 9/9, 2/2",9,9,2,2),
       ("window 9/9, 1/1",9,9,1,1)]
for name,x1,n1,x2,n2 in cases:
    print(f" {name}: SeLow={wilson(x1,n1)[0]:.4f} SpLow={wilson(x2,n2)[0]:.4f} sumrule J_L={sum_rule(x1,n1,x2,n2):.4f} MOVER J_L={mover_J_lower(x1,n1,x2,n2):.4f} CP(97.5) sum J_L={cp_lower(x1,n1)+cp_lower(x2,n2)-1:.4f}")

print("\n== C. Design effect, effective n (equal clusters m), Wilson low for all-correct")
for m in [4,11/3]:
  for rho in [0,0.1,0.3,0.5,1.0]:
    n=36 if m==4 else 33
    deff=1+(m-1)*rho; ne=n/deff
    print(f" n={n} m={m:.3f} rho={rho}: deff={deff:.3f} n_eff={ne:.2f} Wilson low={ne/(ne+Z*Z):.4f}")
# unequal cluster sizes, rho=1: n_eff=(sum m)^2/sum m^2
for ms in [[4]*9,[4,4,4,4,3,3,3,4,4],[3,3,3,3,4,4,4,4,4]]:
    M=sum(ms); ne=M*M/sum(x*x for x in ms); print(f" rho=1 sizes {ms}: n_eff={ne:.3f} (C={len(ms)})")

print("\n== D. Exact one-sided coverage of the Wilson 95% two-sided lower bound (nominal 97.5%), min over p grid")
for n in [3,4,5,9,10,20,36]:
    worst=(1,None)
    for i in range(1,2000):
        p=i/2000
        cov=sum(binom_pmf(k,n,p) for k in range(n+1) if wilson(k,n)[0]<=p)
        if cov<worst[0]: worst=(cov,p)
    # coverage near p=1 region
    near=[(p,sum(binom_pmf(k,n,p) for k in range(n+1) if wilson(k,n)[0]<=p)) for p in (0.5,0.8,0.9,0.95,0.99)]
    print(f" n={n}: min coverage {worst[0]:.4f} at p={worst[1]:.4f}; " + ", ".join(f"p={p}:{c:.3f}" for p,c in near))

print("\n== E. Size of 'sum of Wilson lower bounds > 1' under H0 (Se+Sp=1), independent claims, exact")
def size(n1,n2,rule):
    best=(0,None)
    for i in range(0,201):
        se=i/200; sp=1-se
        P=0.0
        for x2 in range(n2+1):
            p2=binom_pmf(x2,n2,sp)
            if p2==0: continue
            for x1 in range(n1+1):
                if rule(x1,n1,x2,n2): P+=p2*binom_pmf(x1,n1,se)
        if P>best[0]: best=(P,se)
    return best
for n1,n2 in [(36,4),(9,4),(33,3),(9,3),(36,1),(9,2)]:
    s=size(n1,n2,lambda a,b,c,d: sum_rule(a,b,c,d)>0); t=size(n1,n2,lambda a,b,c,d: mover_J_lower(a,b,c,d)>0)
    print(f" n_pos={n1} n_neg={n2}: sum-rule size={s[0]:.5f} at Se={s[1]}; MOVER(J_L>0) size={t[0]:.5f} at Se={t[1]}")

print("\n== F. Clustered positives: 9 windows x 4 positive claims, beta-binomial with ICC rho; 4 negatives independent (1 per window).")
print("   Size under H0 of the per-claim sum rule (36 claims treated as independent) vs window-level rule (n_eff=9).")
def lbeta(a,b): return math.lgamma(a)+math.lgamma(b)-math.lgamma(a+b)
def bb_pmf(m,p,rho):
    if rho==0: return [binom_pmf(k,m,p) for k in range(m+1)]
    if rho>=1: return [1-p]+[0]*(m-1)+[p]
    if p<=0: return [1]+[0]*m
    if p>=1: return [0]*m+[1]
    t=(1-rho)/rho; a=p*t; b=(1-p)*t
    return [math.comb(m,k)*math.exp(lbeta(k+a,m-k+b)-lbeta(a,b)) for k in range(m+1)]
def conv(a,b):
    r=[0.0]*(len(a)+len(b)-1)
    for i,x in enumerate(a):
        for j,y in enumerate(b): r[i+j]+=x*y
    return r
def total_dist(C,m,p,rho):
    d=[1.0]; one=bb_pmf(m,p,rho)
    for _ in range(C): d=conv(d,one)
    return d
for rho in [0,0.2,0.5,1.0]:
    best_claim=(0,None); best_win=(0,None)
    for i in range(1,200):
        se=i/200; sp=1-se
        dist=total_dist(9,4,se,rho)
        pc=0; pw=0
        for x2 in range(5):
            p2=binom_pmf(x2,4,sp)
            for x1,p1 in enumerate(dist):
                if p1==0: continue
                if sum_rule(x1,36,x2,4)>0: pc+=p1*p2
                # window-level effective: x_eff = phat*9, n_eff=9 (Wilson with non-integer counts)
                if wilson(x1/36*9,9)[0]+wilson(x2,4)[0]>1: pw+=p1*p2
        if pc>best_claim[0]: best_claim=(pc,se)
        if pw>best_win[0]: best_win=(pw,se)
    print(f" rho={rho}: per-claim rule size={best_claim[0]:.5f} (Se={best_claim[1]}), n_eff=9 rule size={best_win[0]:.5f} (Se={best_win[1]})")

print("\n== G. Duplicate consistency: Wilson low for D/D concordant duplicated windows")
for D in [1,2,3,5,8,10,16]: print(f" D={D}: {wilson(D,D)[0]:.4f}")
print("\n== H. Kappa/AC1/PABAK for 13/13 agreeing pairs with all values identical (e.g. all true)")
def stats2(a,b,c,d):
    n=a+b+c+d; po=(a+d)/n; p1=(a+b)/n; q1=(a+c)/n
    pe=p1*q1+(1-p1)*(1-q1); kappa=None if pe==1 else (po-pe)/(1-pe)
    q=(p1+q1)/2; pea=2*q*(1-q); ac1=(po-pea)/(1-pea); pabak=2*po-1
    return po,kappa,ac1,pabak
for tab in [(13,0,0,0),(10,0,0,3),(12,1,0,0),(3,0,0,1)]:
    print(f" table a,b,c,d={tab}: po,kappa,AC1,PABAK={stats2(*tab)}")
