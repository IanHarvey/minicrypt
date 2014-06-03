

NDIGITS = 9
MASK = 0x1FFFFFFF
NBITS = 29

def mul256_rs( wordX, wordY ):
    res = [0] * (NDIGITS * 2)
    r64 = 0
    for r in range(0, NDIGITS*2-1):
        if r < NDIGITS:
          sX = r
          sY = 0
        else:
          sX = NDIGITS-1
          sY = (r-NDIGITS+1)
        for i in range(sX-sY+1):
            r64 += wordX[sX] * wordY[sY]
            sX -= 1
            sY += 1
        res[r] = r64 & MASK
        r64 = (r64 >> NBITS)
    assert(r64 <= MASK)
    res[NDIGITS*2-1] = r64
    return res 

def mul(x,y):
    wX = [ (x >> i) & MASK for i in range(0, 255, NBITS) ]
    wY = [ (y >> i) & MASK for i in range(0, 255, NBITS) ]
    wR = mul256_rs(wX, wY)
        
    return sum([ (wR[i] << (i*NBITS)) for i in range(len(wR)) ])
    
tstlist = [ 0, 1, 0x80000000, 0xFFFFFFFF, (1 << 64)-1, (1 << 64),
		(1<<128)-1, (1<<256)-1 ]
    	
if __name__ == '__main__':
    for x in tstlist:
    	for y in tstlist:
    	    res = mul(x,y)
    	    print "x=", hex(x)
    	    print "y=", hex(y)
    	    print "x*y=", hex(x*y)
    	    print "res=", hex(res)
    	    assert(res == x*y)
    