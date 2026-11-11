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
    if mode==14: return 255 if (q-t)%3!=0 else 0
    raise
steps=[0,1,2,3,4,5,6,7,9,10,12,13,14,15,17,20,25,29,30,33,35,36,38,39]
modes=[0,1,2,4,5,6,8,9,10,11,13,14]
for m in modes:
    for right in (True,False):
        rows=[]
        for t in steps:
            rows.append(''.join('%02x'%level(i,right,m,t) for i in range(N)))
        print('  {%d, %s, {%s}},'%(m,'true' if right else 'false',', '.join('"%s"'%r for r in rows)))
print('STEPS', steps)
