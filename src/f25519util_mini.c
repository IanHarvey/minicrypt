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

MCResult F25519_set_mini(F25519_Mini *res, const uint8_t *bytes, size_t len)
{
  int i;
  uint32_t *dp;
  
  if ( len != F25519MINI_MSGSIZE )
    return MC_BAD_LENGTH;
  
  dp = res->digits;  
  for (i=0; i<F25519MINI_MSGSIZE; i+=4)
  {
    *dp++ = bytes[i] | (bytes[i+1] << 8) | (bytes[i+2] << 16) | (bytes[i+3] << 24);
  }
  
  if (mpicmp_mini(res, &F25519_P_mini_) >= 0)
    return MC_BAD_PARAMS;
  return MC_OK;
}

void F25519_setK_mini(F25519_Mini *res, uint32_t n)
{
  mpisetval_mini(res, n);
}


MCResult F25519_get_mini(uint8_t *bytes, size_t len, const F25519_Mini *s)
{
  int i;
  const uint32_t *sp;
  
  if ( len != F25519MINI_MSGSIZE )
    return MC_BAD_LENGTH;
  
  sp = s->digits;  
  for (i=0; i<F25519MINI_MSGSIZE; i+=4)
  {
    uint32_t sd = *sp++;
    bytes[i]   = (uint8_t) (sd & 0xFF);
    bytes[i+1] = (uint8_t) ((sd >> 8) & 0xFF);
    bytes[i+2] = (uint8_t) ((sd >> 16) & 0xFF);
    bytes[i+3] = (uint8_t) ((sd >> 24) & 0xFF);
  }
  return MC_OK;
}
