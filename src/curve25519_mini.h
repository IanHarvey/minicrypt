/*
 * Minicrypt library - Curve25519 public-key crypto primitive
 *
 * See: http://cr.yp.to/ecdh.html
 *
 * https://github.com/IanHarvey/minicrypt
 *
 * This file is placed in the public domain by its author.
 * Note there is NO WARRANTY.
 *
 */
#ifndef CURVE25519_MINI_H
#define CURVE25519_MINI_H

#include "minicrypt.h"
#include "f25519_mini.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  uint8_t bytes[F25519MINI_MSGSIZE];
}
  Curve25519Msg_Mini;

extern MCResult Curve25519_mini(Curve25519Msg_Mini *res, const Curve25519Msg_Mini *a,
	const Curve25519Msg_Mini *P );
/* Multiplies base point P by scalar a, giving result in res.
 * Returns error only if base point is invalid.
 */

#ifdef __cplusplus
}
#endif

#endif

