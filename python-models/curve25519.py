import sys

# Field primitives

P25519 = (1 << 255) - 19

def f_add(a,b):
    return (a+b) % P25519

def f_sub(a,b):
    return (a-b) % P25519

def f_mul(a,b):
    return (a*b) % P25519

def f_sqr(a):
    return (a*a) % P25519

def f_get(s):
    # String tp field element, little-endian
    res = 0
    shift = 0
    for ch in s:
        res += (ord(ch) << shift)
        shift += 8
    return res

def f_put(n):
    # Field element to string, little-endian
    s = ""
    for i in range(32):
        s += chr( n & 0xFF )
        n >>= 8
    return s

def f_modexp_inv(z):
    # Computes z^(-1) by doing
    # z^(2^255-21).
    # This exponent = 0x7FFF....FFEB
    # which is 11111 11111 .... 11111 01011
    
    tmp = f_sqr(z)	# z^2
    tmp = f_sqr(tmp)    # z^4
    tmp = f_mul(tmp, z) # z^5
    tmp = f_sqr(tmp)    # z^10
    res = f_mul(tmp, z) # z^11 (01011)
    tmp = f_sqr(tmp)	# z^20
    tmp = f_mul(tmp,res) # z^31
    
    for i in range(50):
        # shift up tmp's exponent by 5 bits
    	for j in range(5):
    	     tmp = f_sqr(tmp)
    	 # 'Add' in exponent
    	res = f_mul(res, tmp)
    
    return res

# Point operations

class XZPoint:
    def __init__(self, X, Z):
        self.X = X
        self.Z = Z
        
    def reduce(self):
        return f_mul(self.X, f_modexp_inv(self.Z))

def monty_double_add(q, q_, qmqp):
    x = z = xx = zz = xx_ = 0  # These are our temporaries
    
    x   = f_add(q.X, q.Z)
    z   = f_sub(q.X, q.Z)
    
    xx  = f_add(q_.X, q_.Z)
    zz  = f_sub(q_.X, q_.Z)
    
    xx  = f_mul(xx, z)
    zz  = f_mul(x, zz)
    
    xx_ = f_add(xx, zz)
    zz  = f_sub(xx, zz)
    
    q_.X= f_sqr(xx_)
    zz  = f_sqr(zz)
    q_.Z= f_mul(zz, qmqp)
    
    x   = f_sqr(x)
    z   = f_sqr(z)
    q.X = f_mul(x,z)
    
    xx  = f_sub(x,z)
    z   = f_mul(xx, (486662 / 4) )
    z   = f_add(z,x)
    q.Z = f_mul(xx,z)


def scalarMul(a, P):
    # P is x co-ord of point
    assert( P < P25519 )
    qmqp = P
    npqp = XZPoint(P, 1)
    nq   = XZPoint(1, 0)
    # Bit 254 forced to 1
    monty_double_add(npqp, nq, qmqp)
    # Bits 253..3 taken from multiplier
    i = 253
    while i >= 3:
        if (a & (1<<i)):
            monty_double_add(npqp, nq, qmqp)
        else:
            monty_double_add(nq, npqp, qmqp)
        i -= 1
    # Bits 2..0 forced to 0
    while i >= 0:
        monty_double_add(nq, npqp, qmqp)
        i -= 1
    return nq.reduce()

             
def curve25519(a_s, P_s):
    return f_put( scalarMul(f_get(a_s), f_get(P_s)) )

    
if __name__ == '__main__':
   import curve25519_kats
   from binascii import a2b_hex, b2a_hex

   errs = total = 0
   for (a,P,want_aP) in curve25519_kats.tests:
       got_aP = curve25519(a2b_hex(a),a2b_hex(P))
       if b2a_hex(got_aP) != want_aP:
           print "Error"
           print "  want", want_aP
           print "  got",  got_aP
           errs += 1
       total += 1
   print errs, "errors out of", total, "total"
   if (errs > 0):
       sys.exit("Tests failed")
    