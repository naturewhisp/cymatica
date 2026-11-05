import sys,math,array,struct
fn=sys.argv[1]; amp=1e-4; rate=48000
a=array.array('f'); a.frombytes(open(fn,'rb').read())
PI=3.14159265358979323846; w=2.0*PI*997.0/rate
ref=lambda n: struct.unpack('f',struct.pack('f',amp*math.sin(w*n)))[0]
base=48000
win=[a[base+i] for i in range(4800)]
best=None
for k in range(40000,50000):
    e=sum(abs(win[i]-ref(k+i)) for i in range(0,4800,17))
    if best is None or e<best[0]: best=(e,k)
print('best ref idx at capture',base,'=',best[1],'mean abs err',best[0]/(4800/17),'capture-ref offset',base-best[1])
k=best[1]
eq=sum(1 for i in range(4800) if win[i]==ref(k+i)); 
err=[win[i]-ref(k+i) for i in range(4800)]
print('exact in window',eq,'/4800; max err',max(abs(x) for x in err),'rms err',math.sqrt(sum(x*x for x in err)/4800))
num=sum(win[i]*ref(k+i) for i in range(4800)); den=sum(ref(k+i)**2 for i in range(4800)); print('gain',num/den)
print('first samples',[f'{a[i]:.3e}' for i in range(958,972)])
print('ref   samples',[f'{ref(i):.3e}' for i in range(1,8)])
