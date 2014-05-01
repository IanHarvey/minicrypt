/*
 * AES-CCM implementation from minicrypt library
 *
 * https://github.com/IanHarvey/minicrypt
 *
 * This file is placed in the public domain by its author.
 * Note there is NO WARRANTY.
 *
 */
#ifndef AESCCM_MINI_H
#define AESCCM_MINI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "minicrypt.h"
#include "aes_mini.h"

typedef struct
{
  AESMini_ctx actx;
  unsigned L;
  unsigned M;
}
  AESCCMMini_ctx;
  
extern MCResult AESCCMMini_Init(AESCCMMini_ctx *ctx, const uint8_t *key, int keysize, int L, int M);

extern MCResult AESCCMMini_EncryptLength(AESCCMMini_ctx *ctx, size_t plainLen, size_t *cipherLen);
/* Get output message size for encryption of given plaintext */

extern MCResult AESCCMMini_DecryptLength(AESCCMMini_ctx *ctx, size_t cipherLen, size_t *plainLen);
/* Get size of decrypted plaintext given ciphertext size */

extern MCResult AESCCMMini_Encrypt(AESCCMMini_ctx *ctx,
    const uint8_t *plain, size_t plainLen,
    uint8_t *cipher, size_t cipherLen);
/* cipherLen must be value returned from _EncryptLength.
   NB. Will call back MC_GetRandom()
 */

extern MCResult AESCCMMini_Decrypt(AESCCMMini_ctx *ctx,
    const uint8_t *cipher, size_t cipherLen,
    uint8_t *plain, size_t plainLen);
/* plainLen must be value returned from _DecryptLength */



#ifdef __cplusplus
}
#endif

#endif /* AESCCM_MINI_H */

