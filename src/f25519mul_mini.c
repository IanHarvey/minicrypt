/*
 *
 * F25519 field add from Minicrypt library
 *
 * This program is placed into the public domain
 * by its author, Ian Harvey.
 * Note there is NO WARRANTY of any kind.
 */

#define MPIMINI_INTERNAL_API
#include "f25519_mini.h"


static void mul_reduce_approx(uint32_t *dst, uint32_t *src)
// dst is 8 words long
// src is 16 words long
// Forms dst = (src % (2^255-19) + {0..1} * (2^255-19)
{
   int i;

   // We're aiming for dst = src - N*(2^255-19), s.t. dst < 2^255-19
   // Each iteration of the loop forms an approximation N~ = src / (2^255)
    
   for (i=0; i<2; i++)
   {
      int j;
      
      // Do dst <- (src >> 255), which will be our N~
      for (j=0; j < 8; j++)
         dst[j] = (src[j+7] >> 31) | (src[j+8] << 1);
         
      // Do src' -= (N~ * 2^255), i.e. clear all bits > 255
      
      src[7] &= 0x7FFFFFFF;
      for (j=8; j < 16; j++)
        src[j] = 0;
        
      // Adjust: src'' += (N~ * 19), so src'' == src - (N~ * (2^255-19))
      mpi_mulrow_mini_ (src, 19, dst);
      // We can ignore the carry - mulrow will correctly set src[0..8]
      // and we know the true value of src won't be > 20 * (2^255)
      
      // Now src = (src - N~ * (2^255-19))
      // First time round, max value is ~ 19 * (2^255-19)
   }
   
   for (i=0; i<8; i++)
     dst[i] = src[i];
}

#if MPIMINI_DIGITS != 8
#error "F25519_mul3_mini assumes MPIMINI_DIGITS==8"
#endif

void F25519_mul3_mini(F25519_Mini *res, const F25519_Mini *s1, const F25519_Mini *s2)
{
  ULong_Mini mr;
  mpimul_mini(&mr, s1, s2);
  mul_reduce_approx(res->digits, mr.digits);
  F25519_reduce_mini_(res);
}

void F25519_sqr_mini(F25519_Mini *res, const F25519_Mini *s)
{
  F25519_mul3_mini(res, s, s);
}

void F25519_mulK_mini(F25519_Mini *res, const F25519_Mini *s1, uint32_t s2)
{
  UInt_Mini s2l;
  F25519_setK_mini(&s2l, s2);
  F25519_mul3_mini(res, s1, &s2l);
}

/* Test harness ======================================================= */
#ifdef TEST_HARNESS

#include <stdio.h>
#include <string.h>

typedef struct
{
  F25519_Mini a;
  F25519_Mini b;
  F25519_Mini res;
}
  F25519Mul_TV;
  
static const F25519Mul_TV mul_tvs[] =
{
#include "testvectors/f25519mul.inc"
};

static const int mul_tvs_count = sizeof(mul_tvs) / sizeof(F25519Mul_TV);

int main(void)
{
  int i, errs;
  
  errs = 0;
  for (i=0; i < mul_tvs_count; i++)
  {
    const F25519Mul_TV *tv = &mul_tvs[i];
    F25519_Mini res;
   	
    F25519_mul3_mini(&res, &tv->a, &tv->b);
    if ( !F25519_equal_mini(&res, &tv->res) )
    {
      printf("Test #%d failed (1)\n", i);
      errs ++;
      continue;
    }
    
    F25519_copy_mini(&res, &tv->a);
    F25519_mul3_mini(&res, &res, &tv->b);

    if ( !F25519_equal_mini(&res, &tv->res) )
    {
      printf("Test #%d failed (2)\n", i);
      errs ++;
      continue;
    }

    F25519_copy_mini(&res, &tv->b);
    F25519_mul3_mini(&res, &tv->a, &res);

    if ( !F25519_equal_mini(&res, &tv->res) )
    {
      printf("Test #%d failed (3)\n", i);
      errs ++;
      continue;
    }

  }
  
  printf("%d errors out of %d\n", errs, mul_tvs_count);
  return (errs==0) ? 0 : 1;
}

#endif /* TEST_HARNESS */
