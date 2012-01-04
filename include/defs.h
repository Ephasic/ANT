/* Arbitrary Navn Tool -- Miscellaneous Definitions (this file is old)
 * 
 * (C) 2011-2012 Flux-Net
 * Contact us at Dev@Flux-Net.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#ifndef defs_h
#define defs_h
#include "INIReader.h"
#include "extern.h"
#include "flux.h"
/**
 * \include INIReader.h
 * \include extern.h
 * \include flux.h
 */

/**
 * \file defs.h
 * List the constants used throughout the script.
 * If you wanna edit the server, channel, nick, etc that the bot
 * connects to, do so here, NOT in the main code.
*/
#ifdef HAVE_SETJMP_H
jmp_buf sigbuf;
#endif
char **my_av, **my_envp;
bool nofork, dev, protocoldebug, quitting, nocolor, istempnick = false;
Flux::string binary_path, bot_bin, binary_dir, quitmsg, server_name, LastBuf;
char segv_location[255];
time_t starttime = 0;
/**********************Version Variables*************************/
#endif

