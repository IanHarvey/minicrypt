/*
 *
 * MPI utilities from Minicrypt library
 *
 * This program is placed into the public domain
 * by its author, Ian Harvey.
 * Note there is NO WARRANTY of any kind.
 */

#include "mpi_mini.h"

int mpicmp_mini( const UInt_Mini *a, const UInt_Mini *b )
{
  int i;
  for (i=MPIMINI_DIGITS; i-- > 0; )
  {
    if ( a->digits[i] < b->digits[i] )
      return -1;
    if ( a->digits[i] > b->digits[i] )
      return 1;
  }
  return 0;
}
  
void mpisetval_mini( UInt_Mini *res, uint32_t val)
{
  int i;
  res->digits[0] = val;
  
  for (i=1; i<MPIMINI_DIGITS; i++)
    res->digits[i] = 0;
}
