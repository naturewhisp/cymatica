import math
Z=1.959963984540054
def wl(x,n):
    p=x/n; d=1+Z*Z/n; c=(p+Z*Z/(2*n))/d; h=Z*math.sqrt(p*(1-p)/n+Z*Z/(4*n*n))/d; return c-h
print("n  (n-1)/n Wilson low   (n-2)/n")
for n in [4,5,8,10,16,20,30,35]: print(n, round(wl(n-1,n),4), round(wl(n-2,n),4))
C=9
print("distinct cluster-bootstrap resamples C=9:", math.comb(2*C-1,C))
for neg in [1,2,3,4]: print(f"P(resample has no negative window | {neg} of 9 windows have negatives) = {((C-neg)/C)**C:.4f}")
