/* Arbitrary Navn Tool -- Prototypes for ALL global variables and classes
 * 
 * (C) 2011-2012 Azuru
 * Contact us at Dev@Flux-Net.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#ifndef EXTERN_H
#define EXTERN_H
#include "flux.h"
#include <list> //for std::list
#include <new>
/* Prototypes and external variable declarations only */

#include "windows_navn.h" //Include windows crap

/* #define's */
#define E extern CoreExport
#define BUFSIZE 65535
#define CHANNEL_X_INVALID "Channel \2%s\2 is not a valad channel"
#define isvalidnick(c) (isalnum(c) || ((c) >= '\x5B' && (c) <= '\x60') || ((c) >= '\x7B' && (c) <= '\x7D') || (c) == '-')
#define isalphibetic(c) (((c) >= '\x41' && (c) <= '\x5A') || ((c) >= '\x61' && (c) <= '\x7A'))
#define isalphibeticnum(c) ( isalnum(c) || ((c) >= '\x41' && (c) <= '\x5A') || ((c) >= '\x61' && (c) <= '\x7A'))
#define ACCESS_DENIED "Access is Denied."
#define SET_SEGV_LOCATION() snprintf(segv_location, sizeof(segv_location), "%s %d %s", __FILE__, __LINE__, __PRETTY_FUNCTION__);
#define CLEAR_SEGV_LOCATION() segv_location[0]='\0';

#ifdef HAVE_SETJMP_H
E jmp_buf sigbuf;
#endif

/* Classes */
class Channel;
class Log;
class PingTimeoutTimer;
class User;
class NetworkSocket;
class BufferedSocket;
class Socket;
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
  LOG_NORMAL,
  LOG_RAWIO,
  LOG_TERMINAL,
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
E User *FindUser(Network*, const Flux::string&);
E Channel *FindChannel(Network*, const Flux::string&);
E module *FindModule(const Flux::string&);
E Command *FindCommand(const Flux::string&, CommandType);
E Network *FindNetwork(const Flux::string&);
E Bot *FindBot(const Flux::string&);
E bool IsBot(User*);
E Network *FindNetworkByHost(const Flux::string&);

/* extern's */
E Network *FluxNet;
E BotConfig *Config;
E module *LastRunModule;
E GlobalProto *GProto;
E CommandMap Commandsmap;
E CommandMap ChanCommandMap;
E time_t starttime;
E uint32_t usercnt, maxusercnt;
E Flux::string binary_path, bot_bin, binary_dir, quitmsg, LastBuf;
E const Flux::string VERSION_LONG;
E Flux::string getprogdir(const Flux::string&);
E Flux::string DecodeModErr(ModErr err);
E Flux::string isolate(char begin, char end, const Flux::string &msg);
E Flux::string do_strftime(const time_t&, bool short_output = false);
E Flux::string duration(const time_t&);
E Flux::string printfify(const char*, ...);
E Flux::string CondenseVector(const Flux::vector&);
E int randint(int x, int y);
E bool IsTempNick(const Flux::string&);
E bool IsValidChannel(const Flux::string&);
E bool InTerm();
E bool BlakeHash(Flux::string&, const Flux::string&, const Flux::string&);
E bool protocoldebug, dev, nofork, quitting, nocolor, istempnick;
E Flux::vector StringVector(const Flux::string&, char);
E Flux::insensitive_map<module*> Modules;
E std::map<Network*, Channel*> JoinBuffer;
E Flux::insensitive_map<Network*> Networks;
E Flux::map<Network*> NetworkHosts;

/* void's */
E void Rehash();
E void Send_Global(const Flux::string&);
E void Send_Global(const char*, ...);
E void QuitUser(Network*, User*);
E void Fork();
E void sigact(int);
E void SaveDatabases();
E void JoinChansInBuffer(Network*);
E void InitSignals();
E void HandleSegfault(module*);
E void restart(const Flux::string&);
E void ListChans(CommandSource &source);
E void ListUsers(CommandSource &source);
E void process(Network*, const Flux::string&);
E void ProcessJoin(CommandSource&, const Flux::string&);
E void ProcessCommands(CommandSource&, std::vector<Flux::string>&);
E void ReadConfig();

/* Char's */
E char segv_location[255];
E char **my_av, **my_envp;

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
      
#endif