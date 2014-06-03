

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
          count = r+1
        else:
          sX = NDIGITS-1
          sY = (r-NDIGITS+1)
          count = 2*NDIGITS-1-r
        for i in range(count):
            r64 += wordX[sX] * wordY[sY]
            sX -= 1
            sY += 1
        res[r] = r64 & MASK
        r64 = (r64 >> NBITS)
    assert(r64 <= MASK)
    res[NDIGITS*2-1] = r64
    return res 

def toWords(v):
    return [ (v >> i) & MASK for i in range(0, 255, NBITS) ]
    
def toNum(w):
    return sum([ (w[i] << (i*NBITS)) for i in range(len(w)) ])

def mul(x,y):
    return toNum( mul256_rs(toWords(x), toWords(y)) )
    
def mul_reduce_approx(wS):
    for i in range(2):
        wD = [ (wS[j+8] >> 23) | ((wS[j+9] << 6) & MASK)
                 for j in range(9) ]
                 
        wS[8] &= 0x7fffff
        for j in range(9,18):
          wS[j] = 0

        r64 = 0  
        for j in range(9):
          r64 = r64 + wD[j]*19 + wS[j]
          wS[j] = r64 & MASK
          r64 >>= NBITS
        wS[9] = r64
          
    return wS

P25519 = (1 << 255) - 19
    
def mul_mod(x,y):
    wS = mul256_rs(toWords(x), toWords(y))
    res = toNum( mul_reduce_approx(wS) )
    if res >= P25519:
      return res - P25519
    return res
    
tstlist = [ 0, 1, 0x80000000, 0xFFFFFFFF, (1 << 64)-1, (1 << 64),
		(1<<128)-1,
		P25519-0x80000000,
		P25519-1 ]
    	
if __name__ == '__main__':
    for x in tstlist:
    	for y in tstlist:
    	    res = mul_mod(x,y)
    	    ans = (x*y) % P25519
    	    print "x  =", hex(x)
    	    print "y  =", hex(y)
    	    print "x*y=", hex(ans)
    	    print "res=", hex(res)
    	    assert(res == ans)
    