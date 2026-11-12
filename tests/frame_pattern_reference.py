# Independent reference of the 15 frame patterns (written from the spec in the sketch comment, not from the C++ code).
# Prints the hard-coded expectation table used by tests/casino_open_sign_v2_harness.cpp:  python3 tests/frame_pattern_reference.py
N=18
def path(right): return [(14-k)%N for k in range(N)] if right else [(9+k)%N for k in range(N)]
def dist_from_start(i,right):
    start=14 if right else 9
    cw=(start-i)%N
    return min(cw,N-cw)
CORN=[0,5,9,14]
def circ(a,b): d=abs(a-b); return min(d,N-d)
LONG=set(range(0,6))|set(range(9,15))
def hash16(a,b):
    h=(a*40503+b*9973+12345)&0xffffffff
    h^=h>>7
    h=(h*2654435761)&0xffffffff
    h^=h>>13
    return (h>>8)&0xffff
R6=[255,255,128,128,0,0]
R3=[255,128,0]
BL=[255,128,0,128]
def grey_bands(i,q,t):
    ph=(t//36)%4
    if t%36<8: v=128 if ph==1 else 255
    elif ph==0: v=R6[(t-q)%6]
    elif ph==1: v=R6[(t+q)%6]
    elif ph==2: v=R3[(t+i//2)%3] if i%2 else R3[(t-i//2)%3]
    else: v=BL[((q+t//2)//3)%4]
    slot=t//50
    if t%50<8 and hash16(slot,77)%3==0 and i==hash16(slot,5)%18:
        v=[0,128,255][hash16(i,t)%3]
    if t%60<6 and hash16(t//60,91)%2==0 and hash16(t,3)%3!=0: v=0
    return v
def level(i,right,mode,t):
    q=path(right).index(i)
    if mode==0: return 255 if (dist_from_start(i,right)-t)%4<2 else 0
    if mode==1:
        k=(t%18-q)%18; return 255-50*k if k<5 else 0
    if mode==2:
        p=t%16
        if p<10: return 255 if dist_from_start(i,right)<=p else 0
        return 255 if p<14 else 0
    if mode==4: return 200 if i in LONG else (255 if t%8<5 else 20)
    if mode==5: return 255 if (i+t//2)%2 else 0
    if mode==6:
        tri=t%36
        head = tri if tri<18 else 35-tri
        d=(head-q) if tri<18 else (q-head)
        if d==0: return 255
        return 255-70*d if 0<d<4 else 0
    if mode==8: return 255 if (q-t)%3==0 else 0
    if mode==9: return [255,120,200,60][t%10] if t%10<4 else 20
    if mode==10:
        s=t%8
        if s>=6: return 0
        return 255 if min(circ(i,c) for c in CORN)<=s else 0
    if mode==11: return 255 if ((t%10<5)==(i in LONG)) else 0
    if mode==13:
        a=t%18; b=(18-a)%18
        k1=(a-q)%18; k2=(q-b)%18
        f=lambda k:255-80*k if k<3 else 0
        return max(f(k1),f(k2))
    if mode==14: return grey_bands(i,q,t)
    raise
steps=[0,1,2,3,4,5,6,7,9,10,12,13,14,15,17,20,25,29,30,33,35,36,38,39]
modes=[0,1,2,4,5,6,8,9,10,11,13]
for m in modes:
    for right in (True,False):
        rows=[]
        for t in steps:
            rows.append(''.join('%02x'%level(i,right,m,t) for i in range(N)))
        print('  {%d, %s, {%s}},'%(m,'true' if right else 'false',', '.join('"%s"'%r for r in rows)))
print('STEPS', steps)

# Mode 14 (grey bands) has its own step list: it covers the four phases of 36 steps, the phase edges and the broken-pixel bursts.
burst=[t for t in range(3600) if t%50<8 and hash16(t//50,77)%3==0]
slot0=burst[0]//50*50
slot1=burst[len(burst)//2]//50*50
gsteps=[0,1,2,3,7,8,17,35,36,37,43,44,53,71,72,73,79,80,100,107,108,109,115,116,143,144,145,slot0,slot0+1,slot0+3,slot0+7,slot0+8,slot1+2,slot1+5,slot1+9]
blink=[t for t in range(3600) if t%60<6 and hash16(t//60,91)%2==0]
offs=[t for t in blink if hash16(t,3)%3!=0]; ons=[t for t in blink if hash16(t,3)%3==0]
gsteps+= offs[:4]+ons[:3]+[offs[len(offs)//2],ons[len(ons)//2],offs[-1]]
gsteps=sorted(set(gsteps))
print('GREY STEPS', gsteps, 'slots', slot0, slot1, 'broken pixels', hash16(slot0//50,5)%18, hash16(slot1//50,5)%18)
for right in (True,False):
    rows=[''.join('%02x'%level(i,right,14,t) for i in range(N)) for t in gsteps]
    print('  {14, %s, {%s}},'%('true' if right else 'false',', '.join('"%s"'%r for r in rows)))
