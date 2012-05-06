/* Arbitrary Navn Tool -- Prototypes for ALL global variables and classes
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
#ifndef EXTERN_H
#define EXTERN_H
#include "flux.h"
#include <list> //for std::list
#include <new>
/* Prototypes and external variable declarations only */

/* #define's */
#define BUFSIZE 65535
#define CHANNEL_X_INVALID "Channel \2%s\2 is not a valad channel"
#define isvalidnick(c) (isalnum(c) || ((c) >= '\x5B' && (c) <= '\x60') || ((c) >= '\x7B' && (c) <= '\x7D') || (c) == '-')
#define isalphibetic(c) (((c) >= '\x41' && (c) <= '\x5A') || ((c) >= '\x61' && (c) <= '\x7A'))
#define isalphibeticnum(c) ( isalnum(c) || ((c) >= '\x41' && (c) <= '\x5A') || ((c) >= '\x61' && (c) <= '\x7A'))
#define ACCESS_DENIED "Access is Denied."
#define SET_SEGV_LOCATION() snprintf(segv_location, sizeof(segv_location), "%s %d %s", __FILE__, __LINE__, __PRETTY_FUNCTION__);
#define CLEAR_SEGV_LOCATION() segv_location[0]='\0';

#ifdef HAVE_SETJMP_H
extern jmp_buf sigbuf;
#endif

/* Classes */
class Channel;
class Log;
class PingTimeoutTimer;
class User;
class NetworkSocket;
class BufferedSocket;
class Socket;
class tqueue;
class Commands;
class Command;
class Oper;
class Thread;
class module;
class IRCProto;
class GlobalProto;
class ModuleHandler;
class INIReader;
class BotConfig;
class IsoHost;
class Clock;
class TextFile;
class Bot;
class Network;
class ReconnectTimer;
struct CommitMessage;
struct CommandSource;

/* Enumorations */
enum LogType
{
  LOG_DEBUG,
  LOG_MEMORY,
  LOG_NORMAL,
  LOG_RAWIO,
  LOG_TERMINAL,
  LOG_CRITICAL,
  LOG_THREAD,
  LOG_SILENT
};

enum EventResult
{
  EVENT_CONTINUE,
  EVENT_STOP
};

enum CommandType
{
  C_NULL,
  C_CHANNEL,
  C_PRIVATE
};

enum ModErr
{
  MOD_ERR_OK,
  MOD_ERR_MEMORY,
  MOD_ERR_PARAMS,
  MOD_ERR_EXISTS,
  MOD_ERR_NOEXIST,
  MOD_ERR_NOLOAD,
  MOD_ERR_UNKNOWN,
  MOD_ERR_FILE_IO,
  MOD_ERR_EXCEPTION
};

/* Typedef's */
typedef std::map<Flux::string, Command*, ci::less> CommandMap;
typedef std::vector<module*> EventsVector; //Gay g++

/*  Class pointer finder definitions */
extern User *FindUser(Network*, const Flux::string&);
extern Channel *FindChannel(Network*, const Flux::string&);
extern module *FindModule(const Flux::string&);
extern Command *FindCommand(const Flux::string&, CommandType);
extern Network *FindNetwork(const Flux::string&);
extern Bot *FindBot(const Flux::string&);
extern bool IsBot(User*);
extern Network *FindNetworkByHost(const Flux::string&);

/* extern's */
extern Network *FluxNet;
extern BotConfig *Config;
extern module *LastRunModule;
extern GlobalProto *GProto;
extern CommandMap Commandsmap;
extern CommandMap ChanCommandMap;
extern time_t starttime;
extern uint32_t usercnt, maxusercnt;
extern Flux::string binary_path, bot_bin, quitmsg, LastBuf;
extern const Flux::string binary_dir;
extern const Flux::string VERSION_LONG;
extern bool protocoldebug, dev, nofork, quitting, nocolor, istempnick, memdebug;

/* function extern's */
extern Flux::string getprogdir(const Flux::string&, Flux::string &Bot_bin);
extern Flux::string DecodeModErr(ModErr err);
extern Flux::string isolate(char begin, char end, const Flux::string &msg);
extern Flux::string do_strftime(const time_t&, bool short_output = false);
extern Flux::string duration(const time_t&);
extern Flux::string printfify(const char*, ...);
#define CURR_LOCATION printfify("%s:%s", __FILE__, __PRETTY_FUNCTION__)
extern Flux::string CondenseVector(const Flux::vector&);
extern int randint(int x, int y);
extern bool IsValidChannel(const Flux::string&);
extern bool InTerm();
extern bool BlakeHash(Flux::string&, const Flux::string&, const Flux::string&);
extern Flux::vector ParametizeString(const Flux::string&, char);
extern std::map<int, Flux::string> ForwardResolution(const Flux::string&);

/* maps, lists, vectors, etc. */
extern std::list<module*> Modules;
extern Flux::insensitive_map<Network*> Networks;
extern Flux::map<Network*> NetworkHosts;

/* void's */
extern void GarbageCollect();
extern void Rehash();
extern void RenameBot(Network*, const Flux::string&);
extern void Send_Global(const Flux::string&);
extern void Send_Global(const char*, ...);
extern void QuitUser(Network*, User*);
extern void Fork();
extern void sigact(int);
extern void SaveDatabases();
extern void JoinChansInBuffer(Network*);
extern void InitSignals();
extern void HandleSegfault(module*);
extern void restart(const Flux::string&);
extern void ListChans(CommandSource &source);
extern void ListUsers(CommandSource &source);
extern void process(Network*, const Flux::string&);
extern void ProcessJoin(CommandSource&, const Flux::string&);
extern void ProcessCommands(CommandSource&, std::vector<Flux::string>&);
extern void ReadConfig();

/* Char's */
extern char segv_location[255];
extern char **my_av, **my_envp;

/**************************************************************/
/* This is the only #define allowed at the bottom of the file */
#define FOREACH_MOD(y, x) \
if(true) \
{ \
    EventsVector::iterator safei; \
    for (auto _i = ModuleHandler::EventHandlers[y].begin(); _i != ModuleHandler::EventHandlers[y].end(); ) \
    { \
       safei = _i; \
       ++safei; \
       try \
       { \
          (*_i)->x ; \
       } \
       catch (const ModuleException &modexcept) \
       { \
	  Log() << "Exception caught: " << modexcept.GetReason(); \
       } \
        _i = safei; \
    } \
} \
else \
      static_cast<void>(0)

#define FOREACH_RESULT(y, x, v) \
if (true) \
{ \
  std::vector<module*>::iterator safei; \
  v = EVENT_CONTINUE; \
  for (std::vector<module*>::iterator _i = ModuleHandler::EventHandlers[y].begin(); _i != ModuleHandler::EventHandlers[y].end();) \
    { \
      safei = _i; \
      ++safei; \
      try \
      { \
	EventResult res = (*_i)->x ; \
	if (res != EVENT_CONTINUE) { \
	  v = res; \
	  break; \
	} \
      } \
      catch (const ModuleException &modexcept) \
      { \
	Log() << "Exception caught: " << modexcept.GetReason(); \
      } \
      _i = safei; \
    } \
} \
else \
  static_cast<void>(0)


#ifndef __GXX_EXPERIMENTAL_CXX0X__
# error Your compiler MUST support C++11 (C++0x) at least gnu C++ version 4.5!
#endif
#include <atomic>
#include <thread>

#define GetCurrentDir getcwd
#define Delete unlink

#define MODULE_HOOK(x) \
extern "C" module *ModInit(const Flux::string &name) { return new x(name); } \
extern "C" void ModunInit(x **m) { if(*m) delete *m; }

#endif