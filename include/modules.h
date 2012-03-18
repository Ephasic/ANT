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

char **my_av, **my_envp;
bool nofork, dev, protocoldebug, quitting, nocolor, istempnick = false;
Flux::string binary_path, bot_bin, binary_dir, quitmsg, server_name, LastBuf;
char segv_location[255];
time_t starttime = 0;

BotConfig *Config;
Network *FluxNet;
GlobalProto *GProto;
module *LastRunModule; // For crashes

E void startup(int argc, char** argv, char *envp[]);
E void Rehash();
E Flux::string execute(const char *cmd);
E Flux::string urlify(const Flux::string &received);
E Flux::string getprogdir(const Flux::string &dir);


#endif