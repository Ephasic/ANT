/* Arbitrary Navn Tool -- Main include for modules
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#pragma once
#ifndef MODULES_H
#define MODULES_H
#include "module.h"
#include "INIReader.h"
#include "network.h" //We'll solve includes later
#include "bot.h"
#include "tqueue.h"

#ifdef HAVE_SETJMP_H
jmp_buf sigbuf;
#endif

// This is kind of a junk variable file more or less..

char **my_av, **my_envp;
bool nofork = false, dev = false, protocoldebug = false, quitting = false, nocolor = false, istempnick = false;
bool memdebug = false;
Flux::string binary_path, bot_bin, quitmsg, server_name, LastBuf;
const Flux::string binary_dir;
char segv_location[255];
time_t starttime = 0;

BotConfig *Config;
GlobalProto *GProto;
module *LastRunModule; // For crashes

extern void startup(int argc, char** argv, char *envp[]);
extern void Rehash();
extern Flux::string execute(const char *cmd);
extern Flux::string urlify(const Flux::string &received);
extern Flux::string getprogdir(const Flux::string &dir);


#endif