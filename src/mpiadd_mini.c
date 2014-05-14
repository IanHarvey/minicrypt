/*
 *
 * MPI subtract from Minicrypt library
 *
 * This program is placed into the public domain
 * by its author, Ian Harvey.
 * Note there is NO WARRANTY of any kind.
 */

#include "mpi_mini.h"


uint32_t mpiadd_mini( UInt_Mini *res, const UInt_Mini *a, const UInt_Mini *b )
{
    int i;
    uint32_t carry = 0;
    
    for (i=0; i<MPIMINI_DIGITS; i++)
    {
      uint32_t ai = a->digits[i];
      uint32_t bi = b->digits[i];
      uint32_t rL = (ai & 0xFFFF) + (bi & 0xFFFF) + carry;
      uint32_t rH = (ai >> 16) + (bi >> 16) + (rL >> 16);
      res->digits[i] = (rL & 0xFFFF) | (rH << 16);
      carry = rH >> 16;
    } 
      
    return carry;
}

/* Test harness ======================================================= */
#ifdef TEST_HARNESS

#include <stdio.h>
#include <string.h>

typedef struct
{
  UInt_Mini a;
  UInt_Mini b;
  UInt_Mini res;
  uint32_t carry;
}
  MpiAdd_TV;
  
static const MpiAdd_TV add_tvs[] =
{
#include "testvectors/mpiadd.inc"
};

static const int add_tvs_count = sizeof(add_tvs) / sizeof(MpiAdd_TV);

int main(void)
{
  int i, errs;
  
  errs = 0;
  for (i=0; i < add_tvs_count; i++)
  {
    const MpiAdd_TV *tv = &add_tvs[i];
    UInt_Mini res;
    uint32_t carry;
    
    carry = mpiadd_mini(&res, &tv->a, &tv->b);
    if ( memcmp(&res, &tv->res, sizeof(res)) != 0 
         || carry != tv->carry
       )
    {
      printf("Test #%d failed\n", i);
      errs ++;
    }
  }
  
  printf("%d errors out of %d\n", errs, add_tvs_count);
  return (errs==0) ? 0 : 1;
}

#endif /* TEST_HARNESS */
