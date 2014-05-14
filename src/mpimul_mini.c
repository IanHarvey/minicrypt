/*
 *
 * Multiprecision integer multiply from Minicrypt library
 *
 * This program is placed into the public domain
 * by its author, Ian Harvey.
 * Note there is NO WARRANTY of any kind.
 */

#define MPIMINI_INTERNAL_API
#include "mpi_mini.h"

uint32_t mpi_mulrow_mini_ ( uint32_t *dst,  uint32_t sA, const uint32_t *srcB)
{
    uint32_t carry = 0;
    int i, j;

    for (i = 0; i < MPIMINI_DIGITS; i++)
    {
        uint32_t wsum[4];
        uint32_t sB = srcB[i];
        uint32_t rL = (sA & 0xFFFF) * (sB & 0xFFFF);
        uint32_t rM0 = (sA & 0xFFFF) * (sB >> 16);
        uint32_t rM1 = (sA >> 16) * (sB & 0xFFFF);
        uint32_t rH = (sA >> 16) * (sB >> 16);

        wsum[0] = rL & 0xFFFF;
        wsum[1] = (rL >> 16) + (rM0 & 0xFFFF) + (rM1 & 0xFFFF);
        wsum[2] = carry + (rM0 >> 16) + (rM1 >> 16) + (rH & 0xFFFF);
        wsum[3] = (rH >> 16);

        wsum[0] += (dst[i] & 0xFFFF);
        wsum[1] += (dst[i] >> 16);
        wsum[2] += (dst[i+1] & 0xFFFF);
        wsum[3] += (dst[i+1] >> 16);

        for (j=0; j<3; j++)
        {
            wsum[j+1] += (wsum[j] >> 16); 
            wsum[j] &= 0xFFFF;
        }
        dst[i] = (wsum[1]<<16) | wsum[0];
        dst[i+1] = ((wsum[3] & 0xFFFF)<<16) | wsum[2];
        carry = (wsum[3] >> 16);
    }
    return carry;
}

void mpimul_mini( ULong_Mini *res, const UInt_Mini *a, const UInt_Mini *b )
{
    int i;

    for (i = 0; i < 2*MPIMINI_DIGITS; i++)
      res->digits[i] = 0;
    
    for (i = 0; i < MPIMINI_DIGITS; i++)
    {
        mpi_mulrow_mini_ ( &res->digits[i], a->digits[i], b->digits );
        // Note: *in this particular case* it's OK to ignore the carry,
        // as res->digits[sX+MPIMINI_DIGITS] will be zero on entry, so
        // the sum will be at most (slightly less than) this:
        //
        //   00000000 FFFFFFFF...FFFFFFFF   // res->digits on entry
        // + FFFFFFFE FFFFFFFF...00000001   // max 0xFFFFFFFF * b->digits[]
        // = FFFFFFFF 00000000...00000000
    }
}

/* Test harness ======================================================= */
#ifdef TEST_HARNESS

#include <stdio.h>
#include <string.h>

typedef struct
{
  UInt_Mini a;
  UInt_Mini b;
  ULong_Mini res;
}
  MpiMul_TV;
  
static const MpiMul_TV mul_tvs[] =
{
#include "testvectors/mpimul.inc"
};

static const int mul_tvs_count = sizeof(mul_tvs) / sizeof(MpiMul_TV);

int main(void)
{
  int i, errs;
  
  errs = 0;
  for (i=0; i < mul_tvs_count; i++)
  {
    const MpiMul_TV *tv = &mul_tvs[i];
    ULong_Mini res;
    
    mpimul_mini(&res, &tv->a, &tv->b);
    if ( memcmp(&res, &tv->res, sizeof(res)) != 0 )
    {
      printf("Test #%d failed\n", i);
      errs ++;
    }
  }
  
  printf("%d errors out of %d\n", errs, mul_tvs_count);
  return (errs==0) ? 0 : 1;
}

#endif /* TEST_HARNESS */
