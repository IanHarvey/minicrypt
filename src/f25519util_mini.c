/*
 *
 * F25519 field utilities from Minicrypt library
 *
 * This program is placed into the public domain
 * by its author, Ian Harvey.
 * Note there is NO WARRANTY of any kind.
 */

#define MPIMINI_INTERNAL_API
#include "f25519_mini.h"

const F25519_Mini F25519_P_mini_ = 
{
  { 0x1fffffed, 0x1fffffff, 0x1fffffff, 0x1fffffff,
    0x1fffffff, 0x1fffffff, 0x1fffffff, 0x1fffffff,
    0x7fffff
  }
};
 
int32_t F25519_sub_core_mini_ (F25519_Mini *res, const F25519_Mini *s1, const F25519_Mini *s2)
{
  int i;
  int32_t d=0;
  for (i=0; i<F25519MINI_DIGITS-1; i++)
  {
    d += s1->digits[i] - s2->digits[i];
    res->digits[i] = d & F25519MINI_BITMASK;
    d >>= F25519MINI_BITS;
  }
  d += s1->digits[i] - s2->digits[i];
  res->digits[i] = d;
  return d >> F25519MINI_BITS;
}

void F25519_reduce_mini_ (F25519_Mini *res)
{
  while ( F25519_cmp_mini_(res, &F25519_P_mini_) >= 0 )
  {
    F25519_sub_core_mini_( res, res, &F25519_P_mini_ );
  }
}

void F25519_copy_mini(F25519_Mini *res, const F25519_Mini *s)
{
  if (res != s)
    *res = *s;
}

int F25519_cmp_mini_(const F25519_Mini *a, const F25519_Mini *b)
{
  int i;
  for (i=F25519MINI_DIGITS; i-- > 0; )
  {
    if ( a->digits[i] < b->digits[i] )
      return -1;
    if ( a->digits[i] > b->digits[i] )
      return 1;
  }
  return 0;

}

int F25519_equal_mini(const F25519_Mini *a, const F25519_Mini *b)
{
  return F25519_cmp_mini_(a,b) == 0;
}

MCResult F25519_set_mini(F25519_Mini *res, const uint8_t *bytes, size_t len)
{
  int bitpos, digit;
  F25519_setK_mini(res, 0);

  if ( len > F25519MINI_MSGSIZE )
    return MC_BAD_LENGTH;
      
  bitpos = digit = 0;
  while ( len-- > 0 )
  {
    uint8_t val = *bytes++;
      
    res->digits[digit] |= ((val << bitpos) & F25519MINI_BITMASK);
    bitpos += 8;
    if (bitpos > F25519MINI_BITS)
    {
      bitpos -= F25519MINI_BITS; /* Now = no of bits in next digit, 1..8  */
      digit += 1;
      res->digits[digit] = val >> (8-bitpos);
    }
  }

  if (F25519_cmp_mini_(res, &F25519_P_mini_) >= 0)
    return MC_BAD_PARAMS;
  return MC_OK;
}

void F25519_setK_mini(F25519_Mini *res, uint32_t n)
{
  int i;
  res->digits[0] = n & F25519MINI_BITMASK;
  res->digits[1] = (n >> F25519MINI_BITS);
  for (i=2; i < F25519MINI_DIGITS; i++)
    res->digits[i] = 0;
}


MCResult F25519_get_mini(uint8_t *bytes, size_t len, const F25519_Mini *s)
{
  int bitpos, digit;
  
  if ( len != F25519MINI_MSGSIZE )
    return MC_BAD_LENGTH;
    
  bitpos = digit = 0;
  while ( len-- > 0 )
  {
    if ( bitpos + 8 <= F25519MINI_BITS )
    { 
      *bytes++ = (uint8_t)((s->digits[digit] >> bitpos) & 0xFF);
      bitpos += 8;
    }
    else
    {
      uint8_t val = (uint8_t)((s->digits[digit] >> bitpos) & 0xFF);
      bitpos = bitpos + 8 - F25519MINI_BITS; /* Now number of bits to get from next byte */
      digit++;
      val |= (uint8_t)((s->digits[digit] << (8-bitpos)) & 0xFF);
      *bytes++ = val;
    }
  }  
  
  return MC_OK;
}
