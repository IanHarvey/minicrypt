import struct
from binascii import b2a_hex

initialH = [0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
     0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 ]
     
k = [
   0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
   0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
   0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
   0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
   0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
   0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
   0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
   0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
]


def padMsg(msg):
    res = msg + chr(0x80)
    npad = (56-len(res)) & 63
    res += ('\x00' * npad) + struct.pack(">Q", 8*len(msg))
    assert((len(res) % 64)==0)
    return res
    
def add32(*lst):
    return sum(lst) & 0xFFFFFFFF

def not32(r):
    return (~r) & 0xFFFFFFFF

def ROR(a,count):
    return ((a >> count) | (a << (32-count)) & 0xFFFFFFFF)
    
def sha256(msg):
    H = [ x for x in initialH ]
    blocks = padMsg(msg)
    for p in range(0, len(blocks), 64):
        w = list(struct.unpack(">LLLLLLLLLLLLLLLL", blocks[p:p+64]))          
        (a,b,c,d,e,f,g,h) = H
       
        for i in range(64):
            if i < 16:
                w_i = w[i]
            else:
                w_15 = w[(i-15) & 15]
                s0 = ROR(w_15, 7) ^ ROR(w_15, 18) ^ (w_15 >> 3)
                w_2 = w[(i-2) & 15]
                s1 = ROR(w_2, 17) ^ ROR(w_2, 19) ^ (w_2 >> 10)
                w_i = add32(w[i & 15], s0, w[(i-7)&15], s1)
                w[i&15] = w_i
            
            print "w[",i,"] is", hex(w_i)
            S1 = ROR(e,6) ^ ROR(e,11) ^ ROR(e,25)
            ch = ( e & f ) ^ (not32(e) & g)
            temp1 = add32(h, S1, ch, k[i], w_i)
            S0 = ROR(a,2) ^ ROR(a,13) ^ ROR(a,22)
            maj = (a & b) ^ (a & c) ^ (b & c)
            temp2 = add32(S0, maj)
                
            h=g
            g=f
            f=e
            e=add32(d, temp1)
            d=c
            c=b
            b=a
            a=add32(temp1,temp2)
                
        Hres = [a,b,c,d,e,f,g,h]
        for i in range(8):
            H[i] = add32(H[i], Hres[i])
        
    return struct.pack(">LLLLLLLL", H[0], H[1], H[2], H[3], H[4], H[5], H[6], H[7])
    
if __name__ == "__main__":
    import hashlib
    for s in [ "abc", "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" ]:
        print "I get   ", b2a_hex(sha256(s))
        print "They got", b2a_hex(hashlib.sha256(s).digest())

