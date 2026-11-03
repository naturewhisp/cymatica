from math import comb
def tail(n,k,p):
    return sum(comb(n,i)*p**i*(1-p)**(n-i) for i in range(k,n+1))
print("2AFC/ABX one-tailed exact binomial, alpha=.05: min correct, exact alpha, power at true Pc")
print("n  kmin  alpha    P(.70)  P(.75)  P(.80)  P(.90)")
for n in [6,8,10,12,14,16,20,24,30,40]:
    k=next(k for k in range(n+1) if tail(n,k,.5)<=.05)
    print(f"{n:<3}{k:<5}{tail(n,k,.5):.4f}  {tail(n,k,.70):.3f}   {tail(n,k,.75):.3f}   {tail(n,k,.80):.3f}   {tail(n,k,.90):.3f}")
print()
print("Trials needed for 80% power, alpha .05 one-tailed")
for pc in [.65,.70,.75,.80,.90]:
    for n in range(4,400):
        k=next(k for k in range(n+2) if k>n or tail(n,k,.5)<=.05)
        if k<=n and tail(n,k,pc)>=.80 and all((lambda m:(lambda kk:kk<=m and tail(m,kk,pc)>=.80)(next(k2 for k2 in range(m+2) if k2>m or tail(m,k2,.5)<=.05)))(m) for m in range(n,min(n+6,400))):
            print(f"Pc={pc}: n={n}, kmin={k}"); break
print()
# triangle / oddity chance 1/3
print("Oddity (triangle) chance=1/3: min correct for alpha .05")
for n in [6,9,12,15,18,24]:
    k=next(k for k in range(n+1) if tail(n,k,1/3)<=.05)
    print(n,k,round(tail(n,k,1/3),4))
print()
# sign / paired randomization across K blocks with two listeners
print("Min achievable two-tailed p of exact sign test with K pairs all same direction")
for K in [3,4,5,6,8,10]:
    print(K, 2*0.5**K)
print()
print("Beta(1,1) posterior for Pc after s/n: P(Pc>.5), 95% CrI by numeric")
def post(s,n):
    N=20000
    xs=[(i+.5)/N for i in range(N)]
    w=[x**s*(1-x)**(n-s) for x in xs]
    t=sum(w); c=0; lo=hi=None; pgt=sum(wi for x,wi in zip(xs,w) if x>.5)/t
    for x,wi in zip(xs,w):
        c+=wi/t
        if lo is None and c>=.025: lo=x
        if hi is None and c>=.975: hi=x
    return pgt,lo,hi
for s,n in [(9,12),(10,12),(12,16),(13,16),(15,20)]:
    print(s,n,[round(v,3) for v in post(s,n)])
print()
# with p(lapse): number of distinct 2 listener concordance by chance
print("Chance that two independent listeners agree on same pick in K binary choices all (p=.5):")
for K in [4,6,8,10]:
    print(K, 0.5**K)
