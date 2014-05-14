/*
 *
 * MPI subtract from Minicrypt library
 *
 * This program is placed into the public domain
 * by its author, Ian Harvey.
 * Note there is NO WARRANTY of any kind.
 */

#include "mpi_mini.h"


uint32_t mpisub_mini( UInt_Mini *res, const UInt_Mini *a, const UInt_Mini *b )
{
    int i;
    uint32_t carry = 0;
    
    for (i=0; i<MPIMINI_DIGITS; i++)
    {
      int32_t ai = a->digits[i];
      int32_t bi = b->digits[i];
      int32_t rL = (ai & 0xFFFF) - (bi & 0xFFFF) + carry;
      int32_t rH = ((ai >> 16) & 0xFFFF) - ((bi >> 16) & 0xFFFF) + (rL >> 16);
      res->digits[i] = (rL & 0xFFFF) | (rH << 16);
      carry = rH >> 16;
    } 
      
    return (uint32_t)carry;
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
  MpiSub_TV;
  
static const MpiSub_TV sub_tvs[] =
{
#include "testvectors/mpisub.inc"
};

static const int sub_tvs_count = sizeof(sub_tvs) / sizeof(MpiSub_TV);

int main(void)
{
  int i, errs;
  
  errs = 0;
  for (i=0; i < sub_tvs_count; i++)
  {
    const MpiSub_TV *tv = &sub_tvs[i];
    UInt_Mini res;
    uint32_t carry;
    
    carry = mpisub_mini(&res, &tv->a, &tv->b);
    if ( memcmp(&res, &tv->res, sizeof(res)) != 0 
         || carry != tv->carry
       )
    {
      printf("Test #%d failed (1)\n", i);
      errs ++;
      continue;
    }
    
    /* Check with various operands the same as the result */
    res = tv->a;
    carry = mpisub_mini(&res, &res, &tv->b);
    if ( memcmp(&res, &tv->res, sizeof(res)) != 0 
         || carry != tv->carry
       )
    {
      printf("Test #%d failed (2)\n", i);
      errs ++;
      continue;
    }

    res = tv->b;
    carry = mpisub_mini(&res, &tv->a, &res);
    if ( memcmp(&res, &tv->res, sizeof(res)) != 0 
         || carry != tv->carry
       )
    {
      printf("Test #%d failed (3)\n", i);
      errs ++;
      continue;
    }
    
  }
  
  printf("%d errors out of %d\n", errs, sub_tvs_count);
  return (errs==0) ? 0 : 1;
}

#endif /* TEST_HARNESS */
