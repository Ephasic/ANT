/* Arbitrary Navn Tool -- Tiger2 Light Hashing Algorithm
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

#include "Encryption/tiger.h"
#include "log.h"
// %llu
#define tigerhash(str) tiger((byte*)str, strlen(str), res); \
printf("Hash of \"%s\":\n\t%08X%08X %08X%08X %08X%08X\n", \
	str, \
	(word32)(res[0]>>32), \
	(word32)(res[0]), \
	(word32)(res[1]>>32), \
	(word32)(res[1]), \
	(word32)(res[2]>>32), \
	(word32)(res[2]) );
/*
void TigerHash(const Flux::string &str)
{
  word64 res[3];
  char* string = const_cast<char*>(str.c_str());
  tigerhash(string);
}*/