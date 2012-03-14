/* Arbitrary Navn Tool -- Tiger2 Hashing Algorithm
 *
 * (C) 2011-2012 Flux-Net
 * Contact us at Dev@Flux-Net.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

#ifndef _TIGER_H_
#define _TIGER_H_

#ifdef i386
#define ITERATIONS 30
#else
#define ITERATIONS 500
#endif

typedef unsigned long long int word64;
typedef unsigned long word32;
typedef unsigned char byte;

void tiger(byte*, word64, word64*);

#endif // _TIGER_H_
