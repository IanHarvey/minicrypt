/*
 *
 * F25519 field add from Minicrypt library
 *
 * This program is placed into the public domain
 * by its author, Ian Harvey.
 * Note there is NO WARRANTY of any kind.
 */

#define MPIMINI_INTERNAL_API
#include "mpi_mini.h"

void F25519_add3_mini(F25519_Mini *res, const F25519_Mini *s1, const F25519_Mini *s2)
{
    mpiadd_mini(res, s1, s2);
    F25519_reduce_mini_(res);
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
  F25519Add_TV;
  
static const F25519Add_TV add_tvs[] =
{
#include "testvectors/f25519add.inc"
};

static const int add_tvs_count = sizeof(add_tvs) / sizeof(F25519Add_TV);

int main(void)
{
  int i, errs;
  
  errs = 0;
  for (i=0; i < add_tvs_count; i++)
  {
    const F25519Add_TV *tv = &add_tvs[i];
    F25519_Mini res;
   	
    F25519_add3_mini(&res, &tv->a, &tv->b);
    if ( !F25519_equal_mini(&res, &tv->res) )
    {
      printf("Test #%d failed (1)\n", i);
      errs ++;
      continue;
    }
    
    F25519_copy_mini(&res, &tv->a);
    F25519_add3_mini(&res, &res, &tv->b);

    if ( !F25519_equal_mini(&res, &tv->res) )
    {
      printf("Test #%d failed (2)\n", i);
      errs ++;
      continue;
    }

    F25519_copy_mini(&res, &tv->b);
    F25519_add3_mini(&res, &tv->a, &res);

    if ( !F25519_equal_mini(&res, &tv->res) )
    {
      printf("Test #%d failed (3)\n", i);
      errs ++;
      continue;
    }

  }
  
  printf("%d errors out of %d\n", errs, add_tvs_count);
  return (errs==0) ? 0 : 1;
}

#endif /* TEST_HARNESS */