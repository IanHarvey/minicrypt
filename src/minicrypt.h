/*
 * Minicrypt library - common definitions
 *
 * https://github.com/IanHarvey/minicrypt
 *
 * This file is placed in the public domain by its author.
 * Note there is NO WARRANTY.
 *
 */
#ifndef MINICRYPT_H
#define MINICRYPT_H

#include <stdint.h>
#include <stddef.h>

typedef int MCResult;

#define MC_OK               0	/* Success */
#define MC_VERIFY_FAILED    1   /* MAC/Signature verification failed */
#define MC_BAD_LENGTH       2   /* Input/output buffer length was insufficient */
#define MC_BAD_PARAMS       3   /* Parameters are illegal for this algorithm */
#define MC_UNIMPLEMENTED    4   /* Feature is not (yet) implemented */ 
#define MC_RANDOM_FAIL      5   /* Failed to generate random number */

/* Random generation callback: must be supplied by user for certain algorithms */
extern MCResult MC_GetRandom(uint8_t *buffer, size_t length);


#endif
