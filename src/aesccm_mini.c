/*
 * AES-CCM implementation from minicrypt library
 *
 * https://github.com/IanHarvey/minicrypt
 *
 * This file is placed in the public domain by its author.
 * Note there is NO WARRANTY.
 *
 */
  
#include "aesccm_mini.h"

/* ---------------------------------------------- */
  
MCResult AESCCMMini_Init(AESCCMMini_ctx *ctx, const uint8_t *key, int ksz, int L, int M)
{
  if (  M < 4 || M > 16 || (M % 1) != 0 
     || L < 2 || L > 8
     || ! (ksz==AESMINI_128BIT_KEY || ksz==AESMINI_192BIT_KEY || ksz==AESMINI_256BIT_KEY)
     )
    return MC_BAD_PARAMS;
  
  AESMini_Init(&ctx->actx, key, ksz);
  ctx->L = L;
  ctx->M = M; 
  return MC_OK;
}

/* ---------------------------------------------- */

#define NONCE_SZ(ctx) (15-(ctx)->L)
#define TAG_SZ(ctx)   ((ctx)->M)

MCResult AESCCMMini_EncryptLength(AESCCMMini_ctx *ctx, size_t plainLen, size_t *cipherLen)
{
  size_t extra = NONCE_SZ(ctx) + TAG_SZ(ctx);
  size_t cLen = plainLen + extra;
  if (cLen < plainLen) // wrapped - plainLen is invalid
  {
    *cipherLen = 0;
    return MC_BAD_LENGTH;
  }
  *cipherLen = cLen;
  return MC_OK;
}

/* ---------------------------------------------- */

MCResult AESCCMMini_DecryptLength(AESCCMMini_ctx *ctx, size_t cipherLen, size_t *plainLen)
{
  size_t extra = NONCE_SZ(ctx) + TAG_SZ(ctx);
  if (cipherLen < extra)
  {
    *plainLen = 0;
    return MC_BAD_LENGTH;
  }
  *plainLen = cipherLen - extra;
  return MC_OK;
}

/* ---------------------------------------------- */

static int encodeBE( AESCCMMini_ctx *ctx, size_t val, uint8_t *buf16 )
{
  unsigned sz = ctx->L;
  uint8_t *p = buf16 + 15;
  
  while(sz-- > 0)
  {
    *p-- = (val & 0xFF);
    val >>= 8;
  }
  return (val==0);  
}

/* ---------------------------------------------- */

static MCResult AESCCM_common(AESCCMMini_ctx *ctx,
     const uint8_t *nonce,
     const uint8_t *in, uint8_t *out, size_t msgLen,
     uint8_t *tag,
     int isDecrypt)
{
  uint8_t Xi[AESMINI_BLOCK_SIZE];
  uint8_t Ai[AESMINI_BLOCK_SIZE];
  uint8_t Si[AESMINI_BLOCK_SIZE];
  unsigned i; 
  size_t idx;
  const uint8_t *plain = isDecrypt ? out : in;
   
  /* CBCMAC setup */
  Xi[0] = (ctx->L-1) | ((ctx->M-2) << 2);
  for (i=0; i<NONCE_SZ(ctx); i++)
    Xi[i+1] = nonce[i];
  if ( !encodeBE(ctx, msgLen, Xi) )
    return MC_BAD_PARAMS;

  AESMini_ECB_Encrypt(&ctx->actx, Xi, Xi);

  /* Counter block setup */
  Ai[0] = (ctx->L-1);
  for (i=0; i<NONCE_SZ(ctx); i++)
    Ai[i+1] = nonce[i];
    
  /* Process the plaintext */
  idx=1;
  while ( msgLen > 0 )
  {
    size_t n = msgLen > AESMINI_BLOCK_SIZE ? AESMINI_BLOCK_SIZE : msgLen;
    
    encodeBE(ctx, idx, Ai);
    AESMini_ECB_Encrypt(&ctx->actx, Ai, Si);
    
    for (i=0; i<n; i++)
    {
      out[i] = in[i] ^ Si[i];
      Xi[i] ^= plain[i];
    }
    AESMini_ECB_Encrypt(&ctx->actx, Xi, Xi);

    in += n;
    out += n;
    plain += n;
    msgLen -= n;
    idx += 1;
  }
  
  /* Emit the tag */
  encodeBE(ctx, 0, Ai);
  AESMini_ECB_Encrypt(&ctx->actx, Ai, Si); /* Now S_0 */
  for (i=0; i<TAG_SZ(ctx); i++)
    tag[i] = Xi[i] ^ Si[i];
 
  return MC_OK;
}
 
/* ---------------------------------------------- */
 
MCResult AESCCMMini_Encrypt(AESCCMMini_ctx *ctx,
    const uint8_t *plain, size_t plainLen,
    uint8_t *cipher, size_t cipherLen)
{
  size_t tmp;

  if ( AESCCMMini_EncryptLength(ctx, plainLen, &tmp) != MC_OK  ||
       tmp != cipherLen
     )
  {
    return MC_BAD_LENGTH;
  }
   
  /* Get a nonce at start of ciphertext */

  tmp = NONCE_SZ(ctx);
  if ( MC_GetRandom(cipher, tmp) != MC_OK )
    return MC_RANDOM_FAIL;
    
  return AESCCM_common(ctx, 
      cipher, /* Nonce */
      plain, cipher+tmp, plainLen,
      cipher+tmp+plainLen, /* Tag */
      0);
}
/* ---------------------------------------------- */

MCResult AESCCMMini_Decrypt(AESCCMMini_ctx *ctx,
    const uint8_t *cipher, size_t cipherLen,
    uint8_t *plain, size_t plainLen)
{
  size_t tmp;
  uint8_t checkByte;
  uint8_t tag[AESMINI_BLOCK_SIZE];
  MCResult rc;
  unsigned i;
  
  if ( AESCCMMini_DecryptLength(ctx, cipherLen, &tmp) != MC_OK  ||
       tmp != plainLen
     )
  {
    return MC_BAD_LENGTH;
  }
  
  rc = AESCCM_common(ctx,
      cipher, /* Nonce */
      cipher + NONCE_SZ(ctx), plain, plainLen,
      tag,
      1);
      
  if ( rc != MC_OK )
    return rc;
    
  /* Check the tag */
  cipher = cipher + NONCE_SZ(ctx) + plainLen;
  checkByte = 0;
  for (i=0; i<TAG_SZ(ctx); i++)
  {
    checkByte |= (cipher[i] ^ tag[i]);
    /* Should of course be 0 */
  }

  return (checkByte == 0) ? MC_OK : MC_VERIFY_FAILED;
}


/* ==================================================================== */

#ifdef TEST_HARNESS

#include <stdio.h>
#include <string.h>

#define ASSERT_EXPR(a) assert_expr(#a, a, __LINE__)

static int total=0;
static int errs=0;

void assert_expr(const char *what, int ok, int line)
{
  if ( !ok )
  {
    printf("FAIL %s @ line %d\n", what, line);
    errs++;
  }
  total ++;
}
    
typedef struct
{
  int     len;
  uint8_t data[32];
}
  KATBytes;


typedef struct
{
  size_t  len;
  uint8_t data[288];
}
  CCMMessage;

typedef struct
{
  int L;
  int M;
  KATBytes k;
  CCMMessage pt;
  KATBytes nonce;
  CCMMessage ct;
}
  AESCCM_KAT;
  
const int aesccm_kats_count = 20;
const AESCCM_KAT aesccm_kats[20] =
{
  {
    2,
    8,
    { 16, { 0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20 }},
    { 1, { 0x2e }},
    { 13, { 0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d }},
    { 22, { 0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0xe,0x28,0x46,0xce,0x4a,0xff,0xd8,0xb7,0xbf }},
  },
  {
    2,
    8,
    { 16, { 0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e }},
    { 16, { 0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b }},
    { 13, { 0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b }},
    { 37, { 0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x6a,0xe0,0xc,0xae,0x1f,0x32,0x7,0x97,0x94,0x91,0xb0,0x31,0x99,0x55,0xdb,0x78,0xfc,0xef,0x6f,0x2b,0x67,0xfe,0xf9,0x2b }},
  },
  {
    2,
    8,
    { 16, { 0x5c,0x5d,0x5e,0x5f,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b }},
    { 31, { 0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97 }},
    { 13, { 0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78 }},
    { 52, { 0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x8e,0x54,0x42,0xbc,0x1c,0x1d,0x33,0x75,0xba,0xfd,0x4f,0x85,0x2,0x4b,0xb0,0xe6,0xd6,0x7a,0x38,0x3b,0xb5,0x6d,0x29,0x20,0x73,0x33,0x6b,0xae,0x40,0x8a,0xfd,0x9c,0xd2,0xda,0x55,0x75,0x79,0x7b,0x8e }},
  },
  {
    2,
    8,
    { 16, { 0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7 }},
    { 260, { 0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8 }},
    { 13, { 0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4 }},
    { 281, { 0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,0xa7,0xd4,0xe2,0x50,0x2a,0x2f,0x1e,0xc8,0x9d,0x29,0x21,0x2e,0x24,0x98,0x45,0x76,0x3d,0x61,0x0,0x32,0xad,0x4f,0x9b,0x34,0x77,0x84,0xaf,0x3,0x90,0xd6,0x97,0x83,0xad,0x2f,0x2f,0x7b,0x40,0x69,0xb7,0xc7,0x32,0x57,0x4b,0x5,0xd8,0x59,0x7d,0x7d,0x6b,0xb6,0x93,0xb6,0xd7,0x69,0x2d,0x52,0xcc,0x3b,0xc6,0x12,0xe6,0xba,0x80,0xcf,0x2a,0x8a,0xbd,0xee,0x58,0x30,0x80,0x1,0x9d,0xbd,0xa9,0x3b,0xf4,0xc5,0x31,0x1f,0x4e,0xce,0xef,0x3c,0xf7,0xde,0x79,0x4a,0xcd,0x1b,0x88,0x7c,0xf6,0x2e,0xe,0x6e,0xff,0x69,0xcf,0x5a,0x3c,0x63,0xf3,0x17,0x2a,0x1,0x98,0x10,0x42,0xa2,0xc0,0x16,0x83,0x5,0x63,0x50,0xa6,0xa4,0x22,0x19,0xaa,0xc7,0xd7,0xc0,0xda,0x6b,0x96,0x87,0x86,0xc,0x5e,0xf3,0xd5,0x2c,0x62,0xf3,0x99,0x1,0xfa,0xc9,0xc2,0x85,0xb4,0x48,0xe7,0x40,0x5,0x58,0x68,0x42,0x3d,0xaf,0x5,0x84,0x63,0xd9,0x99,0x7f,0xfc,0xa3,0x27,0xaf,0xa2,0xad,0xfb,0xdd,0x55,0x25,0xeb,0x6e,0x68,0x2f,0x4d,0xa1,0x2c,0xb2,0xbf,0xf2,0x2,0x4f,0x45,0xe0,0x7b,0xac,0xa,0x7d,0xba,0x78,0x81,0x45,0xa2,0xae,0x76,0xc4,0xaa,0x93,0xc1,0xb,0x53,0x55,0xb0,0xdd,0xd2,0xa0,0x2e,0xc8,0x2b,0x70,0x8c,0x70,0x5f,0x47,0x20,0x8c,0xfb,0xb1,0xaa,0x77,0xe3,0x65,0x81,0xda,0x92,0x68,0x15,0xa3,0xdb,0x76,0xd1,0xf1,0xd4,0x4c,0xb3,0xd5,0x4d,0x9d,0xf2,0xf2,0x7,0x3e,0xa5,0xd9,0xcf,0xfb,0x4e,0x74,0xaf,0xcc,0x1f,0xf,0xc6,0x3b,0x30,0x50,0xb3,0x93,0xc1,0x3c,0xb6,0xb8,0xf2,0x11,0x86,0x7b,0x68,0xfa,0xd0,0x89 }},
  },
  {
    2,
    12,
    { 16, { 0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8 }},
    { 1, { 0xd6 }},
    { 13, { 0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0xd5 }},
    { 26, { 0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xb3,0xec,0xd6,0x14,0x50,0x4f,0x1c,0x93,0xcc,0xa4,0xcb,0xa4,0xdd }},
  },
  {
    2,
    12,
    { 16, { 0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6 }},
    { 16, { 0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,0x0,0x1,0x2,0x3 }},
    { 13, { 0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,0xf0,0xf1,0xf2,0xf3 }},
    { 41, { 0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,0xf0,0xf1,0xf2,0xf3,0x77,0xda,0x34,0x1a,0xbf,0xd8,0xf5,0xe,0x47,0xde,0x32,0x4b,0x57,0x30,0xd1,0xa3,0xc8,0x15,0xce,0x4f,0xfa,0xba,0xf9,0x6b,0x76,0x77,0xa5,0x40 }},
  },
  {
    2,
    12,
    { 16, { 0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,0x10,0x11,0x12,0x13 }},
    { 31, { 0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f }},
    { 13, { 0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20 }},
    { 56, { 0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x45,0xeb,0x9,0x76,0x2a,0x5e,0x77,0x89,0x8b,0xb7,0x91,0xbd,0x14,0x96,0xbb,0xe3,0x4a,0x72,0xfc,0x32,0x17,0x97,0xf6,0xc,0xc5,0x9f,0x63,0xc4,0xb9,0x93,0x58,0xd3,0x9,0x5f,0x59,0xa0,0xa5,0x8,0x9d,0x29,0x30,0xb0,0x5d }},
  },
  {
    2,
    12,
    { 16, { 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f }},
    { 260, { 0x5d,0x5e,0x5f,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,0x60 }},
    { 13, { 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c }},
    { 285, { 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x2c,0x38,0xdf,0x9b,0x40,0x47,0xfb,0xe8,0x60,0x92,0x60,0x8f,0xb,0xc2,0xab,0xe1,0x9a,0x3c,0xc6,0x10,0x22,0x5a,0x5e,0x7c,0x10,0x72,0xec,0x5,0x73,0x3a,0x33,0x6a,0x11,0x84,0xd0,0xca,0xec,0xe5,0x55,0xb9,0xf0,0x70,0x76,0xef,0x2f,0xa,0x5d,0xa7,0xc1,0x28,0xe6,0xdd,0x3b,0xb1,0x5f,0x9d,0xd4,0xa4,0xa5,0xf,0xeb,0x71,0x60,0x8e,0xdf,0x1b,0xd8,0x9e,0x80,0x85,0x1c,0x22,0x8c,0x2e,0x73,0x87,0xeb,0x76,0x2b,0x64,0x10,0x8c,0xe6,0x99,0x4b,0x22,0xc4,0x93,0xa7,0x63,0xb1,0x1d,0x17,0xc2,0x42,0xbc,0xb7,0xbc,0x41,0xc1,0x8f,0x90,0xfc,0x1b,0xaa,0x48,0x88,0xa4,0x99,0xc,0x39,0x1a,0xad,0x64,0x4b,0x60,0x79,0x5f,0x68,0x42,0xac,0x61,0xf,0xaf,0x3,0x48,0xfb,0xb9,0x23,0xf4,0x45,0x7e,0x4d,0x36,0x7b,0x5e,0xe,0x4a,0x45,0x8e,0x39,0xc3,0xca,0xed,0xd2,0xf6,0xeb,0x7d,0x89,0x26,0x46,0x48,0x93,0x52,0x98,0xa2,0x1b,0x9c,0x92,0x74,0x58,0xb4,0xd7,0xe9,0xf1,0x21,0xef,0xfd,0x35,0x29,0xf0,0x8f,0x86,0xd6,0x86,0xb9,0x6c,0x8b,0xc5,0xf1,0xd4,0xd,0xd8,0xb7,0x78,0x2e,0xb3,0x63,0x2b,0x58,0x81,0x8e,0x6e,0x54,0x65,0x96,0x9f,0x71,0xae,0x4e,0xa6,0xe,0xce,0x4a,0xde,0x1d,0xdd,0xd0,0xbe,0x76,0x91,0x50,0x7f,0x9,0xa4,0x2d,0x97,0x99,0x39,0x3b,0x42,0x18,0xd8,0xa3,0xdf,0x7a,0x49,0x33,0xcb,0x6f,0x1b,0x3f,0x5d,0xca,0xe7,0x1e,0x6d,0x63,0x78,0xc7,0xc1,0xeb,0xc6,0xef,0x14,0xec,0xa0,0x3b,0xb7,0x8f,0x48,0x1d,0x58,0x73,0x7b,0xf6,0x7a,0xd0,0xb2,0x85,0x84,0x2c,0x9c,0xff,0x2,0x2f,0x7a,0x67,0x35,0x1d,0xd,0xff }},
  },
  {
    2,
    14,
    { 16, { 0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70 }},
    { 1, { 0x7e }},
    { 13, { 0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d }},
    { 28, { 0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0xb8,0x43,0xa0,0x86,0xd1,0x7e,0x6b,0xab,0x36,0x9f,0x8f,0xbe,0x85,0xe1,0xf8 }},
  },
  {
    2,
    14,
    { 16, { 0x7f,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e }},
    { 16, { 0x9c,0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab }},
    { 13, { 0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b }},
    { 43, { 0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0xad,0x25,0xf2,0xe6,0xaf,0xbf,0xcb,0x17,0x2c,0x8d,0xeb,0x8d,0x3e,0x4d,0xa4,0x30,0xcf,0xc6,0x7f,0x87,0xce,0x27,0x0,0x8a,0xe8,0x60,0x91,0xc8,0x47,0x94 }},
  },
  {
    2,
    14,
    { 16, { 0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb }},
    { 31, { 0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7 }},
    { 13, { 0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8 }},
    { 58, { 0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0x4d,0xcf,0xe0,0xb2,0x61,0xc7,0x3e,0xe9,0x47,0x9d,0x37,0x75,0x55,0x6b,0x65,0x22,0x77,0x95,0x6f,0x98,0xb,0x3a,0xc6,0xe5,0xa3,0x58,0x6,0x2a,0x9e,0xdf,0x26,0x8c,0xfb,0x3c,0x49,0xa,0x7d,0x2,0xaa,0xa0,0xff,0xea,0x34,0xe3,0x23 }},
  },
  {
    2,
    14,
    { 16, { 0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7 }},
    { 260, { 0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8 }},
    { 13, { 0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,0x0,0x1,0x2,0x3,0x4 }},
    { 287, { 0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,0x0,0x1,0x2,0x3,0x4,0x84,0x83,0xe,0xea,0x5d,0x45,0xaf,0x34,0x35,0x47,0x8d,0x51,0x9d,0xa4,0x65,0x45,0x96,0x26,0xf9,0x10,0xc4,0x59,0xb1,0x1c,0xc0,0x2d,0xf3,0xee,0xa9,0xec,0xf5,0xe1,0xa9,0xb4,0xdc,0x2f,0x49,0x43,0xba,0x8a,0x37,0xc9,0xce,0xbc,0xd2,0x68,0x60,0x20,0x2f,0x33,0xec,0x76,0xa6,0x55,0x82,0x62,0xf4,0x87,0x4b,0x69,0x6e,0xe1,0x61,0xb4,0x45,0xf,0x4d,0xf2,0xba,0x14,0x34,0xe7,0xb4,0xa,0xe3,0xa8,0x36,0xc5,0x0,0x39,0xa8,0xa2,0x25,0x49,0xc3,0x89,0x9e,0x6b,0xa3,0x2f,0x77,0x89,0xba,0xd8,0x47,0x7d,0x61,0x1c,0x84,0x1b,0x76,0x47,0x44,0x23,0xa1,0xc7,0x84,0xc6,0xad,0xe0,0x36,0x97,0x8e,0x25,0xc9,0x47,0x9b,0x1f,0xba,0x2b,0x99,0xc,0xce,0x61,0x42,0xc8,0xc8,0xc5,0x6f,0x87,0xdf,0x9b,0x7f,0x1c,0xac,0x1d,0xb4,0xad,0xcd,0xa9,0xc8,0x29,0xe9,0x6a,0x55,0x23,0x54,0x80,0x2b,0x1a,0xa,0xf5,0xb1,0xfd,0x63,0xaf,0x89,0x58,0xd9,0x1e,0x5e,0x4d,0x3,0x6d,0x74,0x49,0x2,0x15,0x8d,0xd5,0xeb,0x41,0x8a,0x56,0xd4,0xfc,0x17,0x60,0xc2,0x49,0x2,0x8c,0x7d,0xcc,0xee,0x83,0x87,0x7,0x6a,0x7e,0xfa,0xa7,0x10,0x93,0xa3,0x97,0xe2,0xc6,0xbc,0xd1,0x8f,0xd9,0xe4,0x74,0xd7,0x7f,0x1c,0x8b,0xe4,0x32,0xfb,0x6a,0xb1,0xfa,0x63,0xba,0x85,0xac,0x9a,0xd5,0x29,0xda,0xb0,0xfa,0xf9,0x7c,0xb4,0x2f,0x3f,0x77,0x73,0xfb,0x60,0x41,0xec,0x5,0xbc,0x1b,0x46,0x70,0xfe,0xfd,0x2f,0x0,0xdd,0x9b,0xbb,0x26,0x10,0xa7,0x40,0x23,0x23,0x8,0xff,0xd7,0x53,0x34,0xd4,0x3e,0x68,0x50,0xdd,0xd6,0x96,0xeb,0xb0,0x6c,0x6f,0xd8,0xb3,0x8a,0x41,0x34 }},
  },
  {
    4,
    12,
    { 16, { 0x9,0xa,0xb,0xc,0xd,0xe,0xf,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18 }},
    { 1, { 0x24 }},
    { 11, { 0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23 }},
    { 24, { 0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0xfc,0x36,0x67,0x2b,0x2c,0xa4,0xb,0x11,0x49,0x30,0xe4,0xea,0xf9 }},
  },
  {
    4,
    12,
    { 16, { 0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34 }},
    { 16, { 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f }},
    { 11, { 0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f }},
    { 39, { 0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0xe,0xab,0xb2,0x3f,0x99,0x10,0xc1,0xf5,0x4d,0xf7,0x3a,0xcf,0xd6,0x6a,0x77,0x7c,0xdc,0xae,0x15,0x3c,0x6c,0xbd,0x6f,0xb6,0xfc,0x18,0xd2,0x85 }},
  },
  {
    4,
    12,
    { 16, { 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f }},
    { 31, { 0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89 }},
    { 11, { 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a }},
    { 54, { 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x4e,0xa7,0x3a,0x18,0x45,0x17,0xb2,0x2a,0x7a,0xf1,0xb2,0xb7,0xbe,0x1f,0xce,0x13,0xb7,0x22,0x8b,0x49,0x32,0xe4,0xce,0x1d,0x64,0xc5,0x8e,0xc0,0x6c,0xa9,0x5c,0xec,0x7e,0xf5,0x83,0x96,0x2c,0x62,0x24,0x76,0x8b,0x19,0x65 }},
  },
  {
    4,
    12,
    { 16, { 0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99 }},
    { 260, { 0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8 }},
    { 11, { 0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,0xa3,0xa4 }},
    { 283, { 0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,0xa3,0xa4,0xfd,0xf2,0xac,0x11,0xb4,0xce,0x3c,0x39,0x80,0x6a,0x1b,0x4c,0xee,0xc0,0x22,0x33,0x6f,0xe8,0x90,0x3,0x27,0xa0,0xdf,0x1,0xd,0x53,0x68,0x95,0xd8,0x78,0xe9,0x5d,0x70,0x90,0x98,0x65,0xb0,0x4e,0xa8,0xce,0x88,0x44,0x38,0xba,0x2e,0x2a,0xee,0xb8,0xd,0x5f,0xb8,0x98,0x7,0x3c,0xe3,0x4c,0xcf,0x28,0xa8,0x5c,0x1e,0xf6,0x89,0x4b,0xbf,0x4b,0xad,0x98,0xa8,0x19,0xc1,0xa4,0x1f,0x3e,0x6d,0xce,0xa5,0x9b,0x13,0xa5,0x47,0xca,0xab,0x27,0x25,0xd5,0xa4,0xf5,0x5a,0xbc,0x54,0x86,0xb7,0x89,0x20,0xe3,0xd0,0x3f,0x3f,0x31,0x20,0xd5,0xef,0x47,0x24,0x85,0xc6,0x64,0x46,0x73,0x98,0x6e,0x58,0x5b,0xf1,0x1,0x57,0xa9,0x80,0x48,0xf6,0xfb,0x3,0x68,0xe1,0xfd,0xbb,0xaa,0xf7,0x36,0xe8,0xfd,0x91,0x12,0x25,0x57,0x20,0x70,0x5f,0xc2,0x5b,0x33,0xed,0x6e,0xff,0x9a,0xbe,0x12,0xcd,0x18,0x38,0x7f,0x82,0x4b,0xc2,0xb6,0xbd,0xcc,0x69,0x30,0x6c,0x56,0xda,0x8,0x1c,0x4a,0xb1,0xdf,0xfb,0x91,0xa9,0xe3,0x5d,0xe2,0xff,0xff,0x3b,0x95,0xf0,0xaa,0xa7,0x59,0x75,0x4c,0x11,0x64,0xad,0x2,0x44,0xff,0x88,0x6c,0x5,0xef,0x12,0x9a,0x8b,0xce,0xe4,0x3b,0x0,0x88,0xa6,0x78,0xdb,0xd3,0x98,0x52,0x68,0xbe,0xd5,0x38,0x4f,0x9c,0xf6,0x93,0xda,0x2b,0x52,0x9c,0x69,0x5e,0x28,0x39,0xcc,0xb1,0x2f,0xc9,0x49,0x6e,0x85,0xb9,0xe,0xd5,0xce,0xe9,0x6c,0x9b,0xfe,0x2c,0xce,0x1e,0x5e,0x86,0xb3,0x26,0xd7,0x6c,0x37,0x78,0x4a,0xac,0x15,0xa,0xd9,0x30,0x90,0x6a,0xc3,0x29,0xe3,0x5b,0x8,0x83,0x63,0x9d,0x68,0xa,0xf7,0x70,0x52,0x76 }},
  },
  {
    4,
    16,
    { 16, { 0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8 }},
    { 1, { 0xc4 }},
    { 11, { 0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3 }},
    { 28, { 0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0x98,0x88,0x2a,0x22,0xa2,0x5b,0xcd,0x7f,0xe7,0x6b,0x58,0xec,0x4f,0xc5,0x2d,0x69,0x1e }},
  },
  {
    4,
    16,
    { 16, { 0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4 }},
    { 16, { 0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef }},
    { 11, { 0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf }},
    { 43, { 0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,0x68,0x94,0x91,0x1b,0xd3,0x14,0x4c,0x17,0x77,0xf6,0xee,0x10,0x3,0xd8,0x92,0xc4,0x3,0xc7,0x96,0x68,0xfe,0x44,0x65,0xc5,0x3e,0x81,0x6,0x37,0xd6,0x25,0xa3,0x97 }},
  },
  {
    4,
    16,
    { 16, { 0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff }},
    { 31, { 0xb,0xc,0xd,0xe,0xf,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29 }},
    { 11, { 0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa }},
    { 58, { 0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0x70,0x97,0x4d,0x8f,0x7c,0x98,0x39,0xfa,0x44,0xd1,0x5,0xa3,0xbc,0xd3,0x6f,0xc4,0x8d,0x51,0xc7,0xcb,0xf9,0x54,0x10,0x40,0x25,0x95,0x32,0xa,0x22,0xaa,0x25,0x71,0x85,0xd1,0x5,0x71,0xd7,0x60,0xcd,0xa3,0xef,0xf5,0x88,0x79,0xb9,0xd7,0xdd }},
  },
  {
    4,
    16,
    { 16, { 0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39 }},
    { 260, { 0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48 }},
    { 11, { 0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44 }},
    { 287, { 0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x3e,0x71,0x88,0x9a,0xa3,0xad,0x5e,0x63,0xcb,0xe0,0xe5,0xe0,0x2a,0xcf,0xca,0xff,0x87,0x69,0x7b,0xfb,0x7,0x44,0xcc,0xc,0xb4,0x6c,0xe6,0x65,0xf8,0xe4,0xa6,0xf5,0xb8,0x41,0x59,0xee,0xab,0x84,0x9c,0xa3,0xa8,0x59,0x51,0x16,0x2a,0x3c,0xcf,0x16,0xbb,0x9d,0x1a,0x4f,0x9,0xf2,0x1,0x5b,0xa6,0xd5,0x69,0x16,0xf3,0x78,0xad,0x98,0x44,0x1,0x32,0x52,0x7e,0xdf,0xcf,0x9,0xf8,0x7d,0x4b,0x20,0xec,0x65,0x8a,0xcd,0x0,0x1f,0xb4,0xca,0x37,0x1d,0x1,0xcd,0x1c,0x78,0x2a,0x3c,0x9,0x6a,0xf2,0x2d,0x98,0xf5,0x40,0x89,0x14,0xd3,0xaf,0xaa,0x3f,0x1b,0x9b,0x2c,0x89,0xd6,0x2d,0xc7,0x51,0x5d,0xc0,0x76,0x1a,0x84,0xff,0x3e,0xac,0xf7,0xd6,0x3c,0x26,0xc5,0xe6,0x1b,0xfe,0x36,0xe0,0x9d,0x24,0xcc,0x21,0x47,0xa,0xc7,0x20,0x5e,0xd,0x3,0x57,0xd9,0xe4,0xcb,0xa0,0x5e,0x3b,0x69,0x3a,0x89,0x8f,0xe7,0xdf,0x81,0x59,0x31,0x1d,0xf0,0x56,0x44,0x17,0x80,0xe1,0xbf,0xbf,0xec,0xf9,0xa3,0xfe,0x3a,0xe7,0xc3,0x1,0x72,0x87,0x5e,0xc5,0x80,0x49,0x8e,0x59,0x2e,0xcd,0x4b,0x8a,0x6b,0x4c,0xcf,0xe5,0x69,0x7,0x87,0xd8,0xeb,0x34,0x48,0x55,0xf1,0x72,0x8b,0x64,0xc4,0x1c,0xb4,0x3b,0xc6,0x4d,0x77,0x29,0xf3,0x75,0xe3,0x2b,0x7a,0x5e,0x1b,0xb9,0x73,0x1a,0xe7,0xe4,0x93,0x39,0x2a,0x33,0x32,0xba,0xe2,0x1b,0x93,0x1f,0xcf,0x9e,0x4c,0x2,0x68,0xd0,0x3c,0x10,0x83,0xe6,0xe3,0x24,0x9e,0xb0,0xa3,0x27,0x6f,0xdd,0x4d,0x3b,0x9f,0xda,0x9e,0xf0,0xe4,0xf,0x8f,0x77,0x8a,0xc4,0xa8,0xfb,0x14,0x7d,0x28,0xd3,0xe1,0xba,0x2b,0x79,0x60,0xfa,0xb6 }},
  },
};

static int checkdata(const uint8_t *what, const uint8_t *got, size_t sz)
{
  return (memcmp(what, got, sz)==0);
}

static const uint8_t *randBytes;
static size_t randRemain;

MCResult MC_GetRandom(uint8_t *buffer, size_t length)
{
  while ( length-- > 0 )
  {
    if (randRemain < 1)
      return MC_RANDOM_FAIL;
    *buffer++ = *randBytes++;
    randRemain -= 1;
  }
  return MC_OK;
}

static void rng_setup(const AESCCM_KAT *kat)
{
  randBytes  = kat->nonce.data;
  randRemain = kat->nonce.len;
}

static void rng_check(const AESCCM_KAT *kat)
{
  ASSERT_EXPR( randRemain==0 );
}

static void test_aesccm_enc_kats(void)
{
  AESCCMMini_ctx ctx;
  int i;
  int rc;
  CCMMessage cipher;
  
  for (i=0; i<aesccm_kats_count; i++)
  {
    const AESCCM_KAT *kat = &aesccm_kats[i];
    //printf("test %d L=%d M=%d ptlen=%u\n", i, kat->L, kat->M, (unsigned)kat->pt.len);

    rc=AESCCMMini_Init(&ctx, kat->k.data, kat->k.len, kat->L, kat->M);
    ASSERT_EXPR(rc==MC_OK);

    /* Do length check */
    cipher.len = 0xDEADBEEF;
    rc=AESCCMMini_EncryptLength(&ctx, kat->pt.len, &cipher.len);
    ASSERT_EXPR(rc==MC_OK);
    ASSERT_EXPR(cipher.len == kat->ct.len);
    
    /* Now do the encryption */
    rng_setup(kat);
    rc=AESCCMMini_Encrypt(&ctx,
            kat->pt.data, kat->pt.len,
            cipher.data, cipher.len);
    ASSERT_EXPR(rc==MC_OK);
    rng_check(kat);
    
    ASSERT_EXPR(checkdata(kat->ct.data, cipher.data, cipher.len));
  }    
}

static void test_aesccm_dec_kats(void)
{
  AESCCMMini_ctx ctx;
  int i,j;
  int rc;
  CCMMessage plain;
  
  for (i=0; i<aesccm_kats_count; i++)
  {
    const AESCCM_KAT *kat = &aesccm_kats[i];
    //printf("Dec test %d L=%d M=%d ptlen=%u\n", i, kat->L, kat->M, (unsigned)kat->pt.len);
        
    rc=AESCCMMini_Init(&ctx, kat->k.data, kat->k.len, kat->L, kat->M);
    ASSERT_EXPR(rc==MC_OK);

    /* Length query */
    plain.len = 0xF00DCAFE;
    rc=AESCCMMini_DecryptLength(&ctx, kat->ct.len, &plain.len);
    ASSERT_EXPR(rc==MC_OK);
    ASSERT_EXPR(plain.len == kat->pt.len);
    
    /* Do the decrypt */
    rc=AESCCMMini_Decrypt(&ctx,
            kat->ct.data, kat->ct.len,
            plain.data, plain.len);
    ASSERT_EXPR(rc==MC_OK);

    ASSERT_EXPR(checkdata(kat->pt.data, plain.data, plain.len));
    
    /* Check a bogus decrypt */
    for (j=1; j<=4; j++)
    {
      CCMMessage bogusCt = kat->ct;
      switch(j)
      {
        case 1:
            /* Broken nonce */
            bogusCt.data[0] ^= 0x55;
            break;
        case 2:
            /* Broken ciphertext */
            bogusCt.data[kat->nonce.len + kat->pt.len - 3] ^= 0xAA;
            break;
        case 3:
            /* Broken start of tag */
            bogusCt.data[kat->ct.len - kat->M] ^= 0x1;
            break;
        case 4:
            /* Broken end of tag */
            bogusCt.data[kat->ct.len - 1] ^= 0x80;
            break;
      }
      rc=AESCCMMini_Decrypt(&ctx,
            bogusCt.data, bogusCt.len,
            plain.data, plain.len);
      ASSERT_EXPR(rc==MC_VERIFY_FAILED);
    }
  }    
}

int main()
{
  test_aesccm_enc_kats();
  test_aesccm_dec_kats();
  printf("%d errors out of %d\n", errs, total);
  return (errs > 0) ? 255 : 0;
}

#endif