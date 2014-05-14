/*
 *
 * F25519 field utilities from Minicrypt library
 *
 * This program is placed into the public domain
 * by its author, Ian Harvey.
 * Note there is NO WARRANTY of any kind.
 */

#define MPIMINI_INTERNAL_API
#include "mpi_mini.h"

const UInt_Mini F25519_P_mini_ = 
{
  { 0xFFFFFFED, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x7FFFFFFF
  }
};
 

void F25519_reduce_mini_ (UInt_Mini *res)
{
  while ( mpicmp_mini(res, &F25519_P_mini_) >= 0 )
    mpisub_mini(res, res, &F25519_P_mini_);
}

void F25519_copy_mini(F25519_Mini *res, const F25519_Mini *s)
{
  if (res != s)
    *res = *s;
}

int F25519_equal_mini(const UInt_Mini *a, const UInt_Mini *b)
{
  return mpicmp_mini(a,b) == 0;
}