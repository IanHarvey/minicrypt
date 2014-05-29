/*
 * Minicrypt library - F25519 (2^255-19) field operations
 *
 * https://github.com/IanHarvey/minicrypt
 *
 * This file is placed in the public domain by its author.
 * Note there is NO WARRANTY.
 *
 */
#ifndef F25519_MINI_H
#define F25519_MINI_H

#include "minicrypt.h"
#include "mpi_mini.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef UInt_Mini F25519_Mini;

#define F25519MINI_MSGSIZE	32
extern MCResult F25519_set_mini(F25519_Mini *res, const uint8_t *bytes, size_t len);
/* Sets a field element from a byte-block message, in
 * little-endian. 'len' must be exactly F25519MINI_MSGSIZE. */

extern void F25519_setK_mini(F25519_Mini *res, uint32_t n);
/* Sets a field element to a 1-word value */

extern MCResult F25519_get_mini(uint8_t *bytes, size_t len, const F25519_Mini *s);
/* Writes a field element as a byte-block message. len should be F25519MINI_MSGSIZE */

extern void F25519_copy_mini(F25519_Mini *res, const F25519_Mini *s);
/* Does *res = *s. No-op if args are the same */

extern int F25519_equal_mini(const UInt_Mini *a, const UInt_Mini *b);
/* Returns nonzero if *a==*b, or 0 if they're different */

extern void F25519_add3_mini(F25519_Mini *res, const F25519_Mini *s1, const F25519_Mini *s2);
/* Does *res = *s1 + *s2 in field arithmetic. Operands are allowed to be the same */

extern void F25519_sub3_mini(F25519_Mini *res, const F25519_Mini *s1, const F25519_Mini *s2);
/* Does *res = *s1 - *s2 in field arithmetic. Operands are allowed to be the same */

extern void F25519_mul3_mini(F25519_Mini *res, const F25519_Mini *s1, const F25519_Mini *s2);
/* Does *res = *s1 x *s2 in field arithmetic. Operands are allowed to be the same */

extern void F25519_sqr_mini(F25519_Mini *res, const F25519_Mini *s);
/* Does *res = *s x *s in field arithmetic. Operands are allowed to be the same */

extern void F25519_mulK_mini(F25519_Mini *res, const F25519_Mini *s1, uint32_t s2);
/* Does *res = *s1 x s2 in field arithmetic, where s2 is a small integer */


#ifdef __cplusplus
}
#endif

#endif

