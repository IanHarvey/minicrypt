

NDIGITS = 8
MASK = 0xFFFFFFFF

def mul256_rs( wordX, wordY ):
    res = [0] * (NDIGITS * 2)
    r64 = 0
    for r in range(0, NDIGITS*2-1):
        sX = r if (r < NDIGITS) else NDIGITS-1
        sY = 0 if (r < NDIGITS) else (r-NDIGITS+1)
        print "Result Pos", r
        while sX >= 0 and sY < NDIGITS:
            print (sX,sY), '->', (sX + sY)
            r64 += wordX[sX] * wordY[sY]
            sX -= 1
            sY += 1
        res[r] = r64 & MASK
        r64 = (r64 >> 32)
    res[NDIGITS*2-1] = r64
    assert(r64 <= MASK)
    return res 

def mul(x,y):
    wX = [ (x >> i) & 0xFFFFFFFF for i in range(0, NDIGITS*32, 32) ]
    wY = [ (y >> i) & 0xFFFFFFFF for i in range(0, NDIGITS*32, 32) ]
    wR = mul256_rs(wX, wY)
        
    return sum([ (wR[i] << (i*32)) for i in range(0, NDIGITS*2) ])
    
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
    