/*
 * AES-CCM implementation from minicrypt library
 *
 * https://github.com/IanHarvey/minicrypt
 *
 * This file is placed in the public domain by its author.
 * Note there is NO WARRANTY.
 *
 */
#ifndef _AESCCM_H
#define _AESCCM_H

#include "aes_mini.h"
#include <stddef.h>

typedef struct
{
  AESMini_ctx actx;
  unsigned L;
  unsigned M;
}
  AESCCMMini_ctx;
  
int AESCCMMini_Init(AESCCMMini_ctx *ctx, const uint8_t *key, int keysize, int L, int M);

int AESCCMMini_Encrypt(AESCCMMini_ctx *ctx,
    const uint8_t *nonce, size_t nonceLen,
    const uint8_t *plain, size_t plainLen,
    uint8_t *cipher, size_t *cipherLenInOut);

int AESCCMMini_Decrypt(AESCCMMini_ctx *ctx,
    const uint8_t *cipher, size_t cipherLen,
    uint8_t *plain, size_t *plainLenInOut);

#define AESCCMMINI_OK              0
#define AESCCMMINI_INVALID_PARAMS  (-1)
#define AESCCMMINI_BAD_NONCE_SIZE  (-2)
#define AESCCMMINI_BUF_TOO_SHORT   (-3)
#define AESCCMMINI_VERIFY_FAILED   (-4)

#endif

