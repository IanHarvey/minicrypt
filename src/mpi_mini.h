#ifndef MPI_MINI_H
#define MPI_MINI_H
/*
 * Multiprecision-integer and prime-field operationsfrom minicrypt library
 *
 * https://github.com/IanHarvey/minicrypt
 *
 * In the public domain. Note there is NO WARRANTY.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "minicrypt.h"

#define MPIMINI_DIGITS 8
/*
 * Note: this is not a conventional MPI library, in that all values are
 * fixed size; this is to avoid the need for memory allocation during 
 * computation to reduce code size, especially through reduced error handling.
 *
 * Also note, we don't handle signed integers; the UInt_Mini and ULong_Mini types
 * are /unsigned/. Subtraction can therefore return a carry value: for its 
 * intended use in constructing prime field arithmetic primitives, this is not
 * a problem.
 */

typedef struct
{
  uint32_t digits[MPIMINI_DIGITS];
}
  UInt_Mini;
  
typedef struct
{
  uint32_t digits[MPIMINI_DIGITS*2];
}
  ULong_Mini;

extern void mpimul_mini ( ULong_Mini *res, const UInt_Mini *a, const UInt_Mini *b );
/* Multiply two numbers to give double-sized result */

extern uint32_t mpiadd_mini( UInt_Mini *res, const UInt_Mini *a, const UInt_Mini *b );
/* Returns carry-out from top digit (0 or 1) */

extern uint32_t mpisub_mini( UInt_Mini *res, const UInt_Mini *a, const UInt_Mini *b );
/* Returns carry-out from top digit, 0 or 0xFFFFFFFF */

extern int mpicmp_mini( const UInt_Mini *a, const UInt_Mini *b );
/* Returns -1 if a < b; 0 if a==b; 1 if a > b.
 * NB: Not constant-time! */

extern void mpisetval_mini( UInt_Mini *res, uint32_t val);
/* Sets 'res' to a 32-bit value */
   

/* Internal API
 *
 * These functions are for use only within the minicrypt library itself,
 * they may make inconvenient assumptions about the caller, and may change
 * incompatibly between library versions.
 */
#ifdef MPIMINI_INTERNAL_API

extern uint32_t mpi_mulrow_mini_ ( uint32_t *dst,  uint32_t sA, const uint32_t *srcB);
/* Multiplies a row srcB[0..DIGITS-1] by word sA, and adds it into dst[0..DIGITS]
 * Returns the carry out of the top digit (i.e. will need to be added into dst[DIGITS+1]).
 */ 

#endif

#ifdef __cplusplus
}
#endif

#endif
