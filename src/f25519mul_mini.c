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

#define USE_64BIT

#ifdef USE_64BIT

typedef uint64_t U64;

static void u64_sum_row( U64 *sum, const int32_t *s_up, const int32_t *s_dn, int count)
{
  U64 acc = *sum;
  while ( count-- > 0 )
  {
    acc += (U64)(*s_up) * (*s_dn);
    s_up ++;
    s_dn --;
  }
  *sum = acc;
}

static void u64_shift_bits( U64 *val )
{
  *val >>= F25519MINI_BITS;
}

static int32_t u64_mask_bits ( const U64 *src )
{
  return (int32_t) (*src & F25519MINI_BITMASK);
}

static void u64_clear( U64 *val )
{
  *val = 0;
}

#endif 

typedef struct
{
  int32_t digits[F25519MINI_DIGITS*2];
}
  MulResult;
  
#if (F25519MINI_BITS != 29) || (F25519MINI_DIGITS != 9)
#error "mul_reduce_approx is incorrect for F25519MINI_BITS"
#endif

static void mul_reduce_approx(int32_t *dst, int32_t *src)
// dst is 9 words long
// src is 18 words long
// Forms dst = (src % (2^255-19) + {0..1} * (2^255-19)
{
   int i;

   // We're aiming for dst = src - N*(2^255-19), s.t. dst < 2^255-19
   // Each iteration of the loop forms an approximation N~ = src / (2^255)
    
   for (i=0; i<2; i++)
   {
      int j;
      U64 r64;
  
      
      // Do dst <- (src >> 255), which will be our N~
      
      // 255 bits is 8 digits and 23 bits
      
      for (j=0; j < 9; j++)
         dst[j] = (src[j+8] >> 23) | ((src[j+9] << 6) & F25519MINI_BITMASK);
         
      // Do src' -= (N~ * 2^255), i.e. clear all bits > 255
      
      src[8] &= F25519MINI_BITMASK >> 6;
      for (j=9; j < 18; j++)
        src[j] = 0;
        
      // Adjust: src'' += (N~ * 19), so src'' == src - (N~ * (2^255-19))
      u64_clear(&r64);
      for (j=0; j < 9; j++)
      {
        r64 += src[j];
        r64 += (U64)dst[j] * 19;
        src[j] = u64_mask_bits(&r64);
        u64_shift_bits(&r64);
      }
      src[9] = u64_mask_bits(&r64);

      // Now src = (src - N~ * (2^255-19))
      // First time round, max value is ~ 19 * (2^255-19)
   }
   
   for (i=0; i<9; i++)
     dst[i] = src[i];
}

void F25519_mul3_mini(F25519_Mini *res, const F25519_Mini *s1, const F25519_Mini *s2)
{
  MulResult w;
  int r;
  U64 r64;
  
  u64_clear(&r64);
  
  for ( r=0; r < F25519MINI_DIGITS*2 - 1; r++ )
  {
     if ( r < F25519MINI_DIGITS )
     {
       u64_sum_row(&r64, 
                   &s1->digits[0],
                   &s2->digits[r],
                   r+1);
     }
     else
     {
       u64_sum_row(&r64,
                   &s1->digits[r-F25519MINI_DIGITS+1],
                   &s2->digits[F25519MINI_DIGITS-1],
                   2*F25519MINI_DIGITS-1-r);
     }
     w.digits[r] = u64_mask_bits(&r64);
     u64_shift_bits(&r64);
  }

  w.digits[r] = u64_mask_bits(&r64);
  
  mul_reduce_approx(res->digits, w.digits);
  F25519_reduce_mini_(res);
}

void F25519_sqr_mini(F25519_Mini *res, const F25519_Mini *s)
{
  F25519_mul3_mini(res, s, s);
}

void F25519_mulK_mini(F25519_Mini *res, const F25519_Mini *s1, uint32_t s2)
{
  F25519_Mini s2l;
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
