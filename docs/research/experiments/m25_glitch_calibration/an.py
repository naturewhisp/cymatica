import sys,math,array
fn=sys.argv[1]; amp=float(sys.argv[2]); rate=int(sys.argv[3]) if len(sys.argv)>3 else 48000
a=array.array('f'); a.frombytes(open(fn,'rb').read())
import struct
w=2*math.pi*997.0/rate
PI=3.14159265358979323846
w=2.0*PI*997.0/rate
def ref(n):
    return struct.unpack('f',struct.pack('f',amp*math.sin(w*n)))[0]
N=len(a)
# first nonzero
s=next(i for i,v in enumerate(a) if v!=0.0)
print('samples',N,'first nonzero at',s,'value',a[s],'ref(0)',ref(0),'ref(1)',ref(1))
# locate start in ref (ref(0)=0 so first nonzero is ref(1))
j=1; i=s; exact=0; inserted=0; mism=0; gaps=[]; cur=0; first_mis=None; maxerr=0.0
while i<N:
    v=a[i]; r=ref(j)
    if v==r:
        if cur: gaps.append((i-cur,cur)); cur=0
        exact+=1; i+=1; j+=1
    elif v==0.0:
        cur+=1; inserted+=1; i+=1
    else:
        mism+=1
        e=abs(v-r); maxerr=max(maxerr,e)
        if first_mis is None: first_mis=(i,j,v,r)
        i+=1; j+=1
print('exact',exact,'inserted_zero',inserted,'mismatch',mism,'first_mismatch',first_mis,'max_abs_err',maxerr,'final ref idx',j)
print('gaps (start_frame, frames, ms):',[(g[0],g[1],round(1000*g[1]/rate,2)) for g in gaps][:24], 'n=',len(gaps))
