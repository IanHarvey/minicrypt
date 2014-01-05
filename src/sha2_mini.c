/*
 * This program is placed into the public domain
 * by its author, Ian Harvey.
 * Note there is NO WARRANTY of any kind.
 */
 
#include "sha2_mini.h"

#include <assert.h>
#include <string.h>

/* SHA-256 Main compression function */

static const uint32_t K[64] =
{
   0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
   0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
   0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
   0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
   0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
   0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
   0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
   0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static uint32_t ROR(uint32_t v, unsigned n)
{
  return (v >> n) | (v << (32-n));
}

static void get_BE32(const uint8_t *bytes, uint32_t *words, unsigned nwords)
{
  while ( nwords-- > 0 )
  {
    *words++ = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2]<<8) | bytes[3];
    bytes += 4;
  }
}

static void put_BE32(const uint32_t *words, uint8_t *bytes, unsigned nwords)
{
  while ( nwords-- > 0 )
  {
    uint32_t w = *words++;
    bytes[0] = (w >> 24) & 0xFF;
    bytes[1] = (w >> 16) & 0xFF;
    bytes[2] = (w >> 8) & 0xFF;
    bytes[3] = (w & 0xFF);
    bytes += 4;
  }
}

static void SHA256Mini_compress(uint32_t *H, const uint8_t *msg)
/* H is 8 words, msg is 64 bytes */
{
  uint32_t W[16];
  unsigned i;
  
  get_BE32(msg, W, 16);
  
  uint32_t a,b,c,d,e,f,g,h;
  
  a=H[0]; b=H[1]; c=H[2]; d=H[3];
  e=H[4]; f=H[5]; g=H[6]; h=H[7];
  
  for (i=0; i<64; i++)
  {
    uint32_t temp1, temp2, W_i;
    
    if ( i < 16 )
      W_i = W[i];
    else
    {
      temp1 = W[(i-15) & 15];
      temp1 = ROR(temp1, 7) ^ ROR(temp1, 18) ^ (temp1 >> 3);

      temp2 = W[(i-2) & 15];
      temp2 = ROR(temp2, 17) ^ ROR(temp2, 19) ^ (temp2 >> 10);
      W_i = W[i & 15] + W[(i-7) & 15] + temp1 + temp2;
      W[i & 15] = W_i;
    }
    
    temp1 = h + K[i] + W_i;
    temp1 += ROR(e,6) ^ ROR(e,11) ^ ROR(e,25);
    temp1 += (e & f) ^ (~e & g);
    
    temp2 = ROR(a,2) ^ ROR(a,13) ^ ROR(a,22);
    temp2 += (a & b) ^ (a & c) ^ (b & c);
    
    h=g;  g=f;  f=e;  e = d + temp1;
    d=c;  c=b;  b=a;  a = temp1 + temp2;
  }
  
  H[0] += a;
  H[1] += b;
  H[2] += c;
  H[3] += d;
  H[4] += e;
  H[5] += f;
  H[6] += g;
  H[7] += h;
}


static const uint32_t Hinit[8] = 
{
  0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
  0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

void SHA256Mini_Init(SHA256Mini_ctx *ctx)
{
  int i;

  for(i=0; i<8; i++)
    ctx->H[i] = Hinit[i];
    
  ctx->count = 0; 
}

void SHA256Mini_Update(SHA256Mini_ctx *ctx, const void *pvmsg, size_t remain)
{
  const uint8_t *msg = (const uint8_t *)pvmsg;
  
  while ( remain > 0 )
  {
    uint32_t got = (ctx->count & 63);
    uint32_t space = 64-got;
    uint32_t taken;
    
    if ( remain >= space ) 
    {
      if ( got==0 ) // Nothing in buffer, avoid copy
        SHA256Mini_compress(ctx->H, msg);
      else
      {    
        memcpy( ctx->msgbuf + got, msg, space );
        SHA256Mini_compress(ctx->H, ctx->msgbuf);
      }
      taken = space;
    }
    else
    {
      // Will have to buffer until next time
      memcpy( ctx->msgbuf + got, msg, remain );
      taken = remain;
    }
    ctx->count += taken;
    msg += taken;
    remain -= taken;
  }
}

void SHA256Mini_Final(SHA256Mini_ctx *ctx, uint8_t *out)
{
  /* Pad last block. Note buffer is never totally full,
     as we call _compress() every time it is.
  */
  uint32_t got = (ctx->count & 63);
 
  ctx->msgbuf[got++] = 0x80;
  
  if ( got > 56 )
  {
    /* Not enough */
    memset(ctx->msgbuf + got, 0, 64-got);
    SHA256Mini_compress(ctx->H, ctx->msgbuf);
    got = 0;
  }
  memset( ctx->msgbuf + got, 0, 64-got );
  
  /* Write out size, big-endian,
     to msgbuf bytes 56..63.
     Cunningly, we don't need to know how big a size_t is;
     this depends on all bytes being zeroed by the above
     memset.
  */
     
  size_t nbits = ctx->count << 3;
  assert( (nbits >> 3) == ctx->count ); // Check for size_t overflow
  
  uint8_t *p = ctx->msgbuf + 63;
  while ( nbits > 0 )
  {
    *p-- = (uint8_t)(nbits & 0xFF);
    nbits >>= 8;
  }
  
  SHA256Mini_compress(ctx->H, ctx->msgbuf);

  put_BE32(ctx->H, out, 8);
}

void SHA256Mini(const void *msg, size_t msglen, uint8_t *out32)
{
  SHA256Mini_ctx c;
  SHA256Mini_Init(&c);
  SHA256Mini_Update(&c, msg, msglen);
  SHA256Mini_Final(&c, out32);
}

/* ----------------------------------------------------------------- */

#ifdef TEST_HARNESS

#include <stdio.h>

typedef struct
{
  const char *in;
  uint8_t out[SHA256MINI_HASHLEN];
}
   TestVector;

static const TestVector tvs[] =
{
  { "",
    { 0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
      0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
      0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
      0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55
    }
  },
  { "abc",
    { 0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
      0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
      0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
      0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
    }
  },
  { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 
    { 0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8,
      0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
      0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67,
      0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1
    }
  },
  
  { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq7",
    { 0x55, 0xb8, 0x3a, 0x54, 0x41, 0x1b, 0x88, 0xfa,
      0x8e, 0x9c, 0x79, 0x10, 0xc7, 0x1c, 0x1b, 0xe4,
      0xea, 0xa9, 0x57, 0x6b, 0x0a, 0x8a, 0xc7, 0x4c,
      0x8c, 0x87, 0xfc, 0x5b, 0x71, 0x09, 0x60, 0x6e
    },
  },
  
  { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq78901234",
    { 0x3d, 0x07, 0x8b, 0xe4, 0xa2, 0xb2, 0x13, 0x87,
      0x7a, 0xb1, 0x18, 0x0a, 0x29, 0x31, 0xf7, 0x0f,
      0x2d, 0x78, 0xd0, 0xfa, 0xa0, 0xc5, 0x06, 0x78,
      0x2b, 0x90, 0x1a, 0x1a, 0x7e, 0xa9, 0x0b, 0xf5
    },
  },

  { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq78901234"
    "ABCDBCDECDEFDEFGEFGHFGHIGHIJHIJKIJKLJKLMKLMNLMNOMNOPNOPQ(){};:!",
    {
      0x02, 0x46, 0xFC, 0xB7, 0xDF, 0x32, 0x90, 0x00,
      0xF4, 0xE7, 0xC0, 0x54, 0xFA, 0x40, 0x8E, 0x2B,
      0x07, 0xB9, 0xB2, 0x8F, 0xD9, 0x73, 0xB0, 0xC8,
      0x9E, 0xB3, 0x5F, 0x8C, 0x05, 0xB2, 0xDA, 0xE0
    },
  },
  
  { NULL, { 0 } }
};

int main()
{
  const TestVector *tv;
  int fails=0;
  
  for (tv=&tvs[0]; tv->in != NULL; tv++)
  {
    uint8_t buf[SHA256MINI_HASHLEN];
    size_t l = strlen(tv->in);
    printf("Length %5ld: ", (unsigned long)l);
    SHA256Mini(tv->in, l, buf);
    if ( memcmp(buf, tv->out, 32) != 0 )
    {
      fails++;
      printf("FAIL\n");
    }
    else  
      printf("OK\n");
  }
  
  return fails ? 1 : 0;
}  

#endif
