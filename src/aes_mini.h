#ifndef AES_MINI_H
#define AES_MINI_H
/*
 * AES (Rijndael) implementation from minicrypt library
 *
 * https://github.com/IanHarvey/minicrypt
 *
 * In the public domain. Note there is NO WARRANTY.
 *
 * When building this, you can #define:
 *  AESMINI_128BIT_ONLY to remove support for 192-bit and 256-bit keys, and/or
 *  AESMINI_ENCRYPT_ONLY to remove support for the AES decryption primitive
 */

#include <stdint.h>

#define AESMINI_BLOCK_SIZE 16

#ifdef AESMINI_128BIT_ONLY
# define AES_NROUNDKEYS 44
#else
# define AES_NROUNDKEYS 60
#endif

typedef struct
{
  int nrounds;
  uint32_t roundkeys[AES_NROUNDKEYS];
}
  AESMini_keys;

typedef struct
{
  AESMini_keys enc;
#ifndef AESMINI_ENCRYPT_ONLY
  AESMini_keys dec;
#endif
}
  AESMini_ctx;


/* Initialises a context for a key. Key size is given in *bytes*  */
extern void AESMini_Init(AESMini_ctx *ctx, const uint8_t *key, int nkeybytes);
#define AESMINI_128BIT_KEY	16
#define AESMINI_192BIT_KEY	24
#define AESMINI_256BIT_KEY	32

/* Single ECB encrypt primitive. 'plain' and 'cipher' must be AESMINI_BLOCK_SIZE each */
extern void AESMini_ECB_encrypt(AESMini_ctx *ctx, const uint8_t *plain, uint8_t *cipher);

/* Single ECB decrypt primitive. 'plain' and 'cipher' must be AESMINI_BLOCK_SIZE each */
#ifndef AESMINI_ENCRYPT_ONLY
extern void AESMini_ECB_decrypt(AESMini_ctx *ctx, const uint8_t *cipher, uint8_t *plain);
#endif

#endif
