/* Arbitrary Navn Tool -- Primary functions file (This file is old)
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
// This cannot include modules.h because it will multi-define stuff and make g++ shit bricks
#include "module.h"
#include "INIReader.h"
// #include "modules.h"
#include "network.h" //We'll solve includes later
#include "bot.h"

/**Runtime directory finder
 * This will get the bots runtime directory
 * @param getprogdir(const Flux::string dir)
 */
Flux::string getprogdir(const Flux::string &dir, Flux::string &Bot_bin = bot_bin)
{
  char buffer[FILENAME_MAX];
  if (GetCurrentDir(buffer, sizeof(buffer)))
  {
    Flux::string remainder = dir;
    Bot_bin = remainder;
    Flux::string::size_type n = Bot_bin.rfind("/");
    Flux::string fullpath;

    if (Bot_bin[0] == '/')
      fullpath = Bot_bin.substr(0, n);
    else
      fullpath = Flux::string(buffer) + "/" + bot_bin.substr(0, n);

    Bot_bin = Bot_bin.substr(n + 1, remainder.length());
    return fullpath;
  }
  return "/";
}

/**
 * \fn Flux::string execute(const char *cmd)
 * \brief Sends a command to the OS
 * \param cmd A command
 * \return A Flux::string containing the response from the OS.
 */
Flux::string execute(const char *cmd)
{
  /*
   * Roses are red,
   * Violets are blue,
   * I read StackOverflow
   * And so do you!
   */
  FILE* pipe = popen(cmd, "r");

  if (!pipe)
    return "";

  char buffer[128];
  Flux::string result = "";

  while(!feof(pipe))
  {
    if(fgets(buffer, 128, pipe) != NULL)
      result += buffer;
  }

  pclose(pipe);
  return result;
}
/**
 * \fn static void restart(Flux::string reason)
 * \brief Restart the bot process
 * \param reason The reason for the restart
 */
void restart(const Flux::string &reason)
{
  char CurrentPath[FILENAME_MAX];
  GetCurrentDir(CurrentPath, sizeof(CurrentPath));
  FOREACH_MOD(I_OnRestart, OnRestart(reason));

  for(auto it : Networks)
  {
    Network *n = it.second;
    if(reason.empty())
    {
      Log() << "Restarting: No Reason";
      if(n->b && n->b->ircproto)
	n->Disconnect("Restarting: No Reason");
    }
    else
    {
      Log() << "Restarting: " << reason;
      if(n->b && n->b->ircproto)
	n->Disconnect("Restarting: %s", reason.c_str());
    }
  }

  chdir(CurrentPath);
  int execvpret = execvp(my_av[0], my_av);

  if(execvpret > 0)
    throw CoreException("Restart Failed, Exiting");

  Delete(Config->PidFile.c_str());
  exit(1);
}

/**
 * \fn void Rehash()
 * \brief Reload the bot config file
 * \param boolean this boolean tells rehash if we are starting from start or not
 */
void Rehash()
{
  Log() << "Rehashing Configuration File";
  try
  {
    BotConfig *configtmp = Config;
    Config = new BotConfig(configtmp);
    delete configtmp;

    if(!Config)
      throw ConfigException("Could not read config.");

    FOREACH_MOD(I_OnReload, OnReload());
    LoadModules();
  }
  catch(const ConfigException &ex)
  {
    Log() << "Configuration Exception Caught: " << ex.GetReason();
  }
}

/**
 * \fn static void remove_pidfile()
 * \brief Removes the PID file on exit
 */
static void remove_pidfile()
{
  Delete(Config->PidFile.c_str());

  // We delete our config pointer here because this is the
  // last function called before we actually exit.
  if(Config)
    delete Config;
}
/**
 * \fn static void WritePID()
 * \brief Write the bots PID file
 */
static void WritePID()
{
  //logging to a text file and making the PID file.
  if(Config->PidFile.empty())
    throw CoreException("Cannot write PID file, no PID file specified.");

  FILE *pidfile = fopen(Config->PidFile.c_str(), "w");
  if(pidfile)
  {
    fprintf(pidfile, "%d\n", static_cast<int>(getpid()));
    fclose(pidfile);
    atexit(remove_pidfile);
  }
  else
    throw CoreException("Can not write to PID file "+Config->PidFile);
}

class CommandLineArguments
{
protected:
  Flux::map<Flux::string> Arguments;
public:
  CommandLineArguments(int ac, char **av)
  {
    for(int i = 1; i < ac; ++i)
    {
      Flux::string argsarg = av[i];
      Flux::string param;
      while(!argsarg.empty() && argsarg[0] == '-')
	argsarg.erase(argsarg.begin());

      size_t t = argsarg.find('=');
      if(t != Flux::string::npos)
      {
	param = argsarg.substr(t+1);
	argsarg.erase(t);
      }

      if(argsarg.empty())
	continue;

      Arguments[argsarg] = param;
    }
  }

  bool HasArg(const Flux::string &name, char shortname = '\0')
  {
    Flux::string Cppisstupidrighthere;
    return this->HasArg(name, shortname, Cppisstupidrighthere);
  }

  bool HasArg(const Flux::string &name, char shortname, Flux::string &args)
  {
    args.clear();

    for(Flux::map<Flux::string>::iterator it = this->Arguments.begin(); it != this->Arguments.end(); ++it)
    {
      if(it->first.equals_ci(name) || it->first[0] == shortname)
      {
	args = it->second;
	return true;
      }
    }
    return false;
  }
};

/**This is the startup sequence that starts at the top to the try loop
 * @param startup(int, char)
 */
void startup(int argc, char** argv, char *envp[])
{
  SET_SEGV_LOCATION();
  InitSignals();
  Config = nullptr;
  my_av = argv;
  my_envp = envp;
  starttime = time(NULL); //for bot uptime

  Flux::string bindir = getprogdir(Flux::string(argv[0]));
  if(bindir[bindir.length() - 1] == '.')
    bindir = bindir.substr(0, bindir.length() - 2);
  binary_dir = bindir;

  Config = new BotConfig(NULL);
  if(!Config)
    throw CoreException("Config Error.");

  Flux::string dir = argv[0];
  Flux::string::size_type n = dir.rfind('/');
  dir = "." + dir.substr(n);

  //gets the command line parameters if any.
  CommandLineArguments args(argc, argv);

  if(args.HasArg("developer", 'd'))
  {
    dev = nofork = true;
    Log(LOG_DEBUG) << "ANT Commit System is started in Developer mode.";
  }

  if(args.HasArg("nofork", 'n'))
  {
    nofork = true;
    Log(LOG_DEBUG) << "ANT Commit System is started With No Forking enabled.";
  }

  if(args.HasArg("readonly", 'r'))
  {
    readonly = true;
    Log(LOG_DEBUG) << "ANT Commit System is started With read only databases enabled.";
  }

  if(args.HasArg("memorydebug", 'm'))
  {
    memdebug = true;
    Log(LOG_DEBUG) << "ANT Commit System is started With memory debug enabled.";
  }

  if(args.HasArg("help", 'h'))
  {
    Log(LOG_TERMINAL) << "ANT Internet Relay Chat Commit Bot system v" << VERSION;
    Log(LOG_TERMINAL) << "Usage: " << dir << " [options]";
    Log(LOG_TERMINAL) << "-h, --help";
    Log(LOG_TERMINAL) << "-d, --developer";
    Log(LOG_TERMINAL) << "-n, --nofork";
    Log(LOG_TERMINAL) << "-p, --protocoldebug";
    Log(LOG_TERMINAL) << "-c, --nocolor";
    Log(LOG_TERMINAL) << "-m, --memorydebug";
    Log(LOG_TERMINAL) << "-r, --readonly";
    Log(LOG_TERMINAL) << "-v, --version";
    Log(LOG_TERMINAL) << "See --version for full version information";
    Log(LOG_TERMINAL) << "This bot does have Epic Powers.";
    exit(0);
  }

  if(args.HasArg("version", 'v'))
  {
    Log(LOG_TERMINAL) << "Arbitrary Navn Tool IRC Commit System C++ Bot Version " << VERSION_FULL;
    Log(LOG_TERMINAL) << "This bot was programmed from scratch by Justasic and Lordofsraam.";
    Log(LOG_TERMINAL) << "";
    Log(LOG_TERMINAL) << "IRC: irc.Azuru.net #Computers";
    Log(LOG_TERMINAL) << "WWW: http://www.Azuru.net";
    Log(LOG_TERMINAL) << "Email: Development@Azuru.net";
    Log(LOG_TERMINAL) << "Git: arbitrary-navn-tool.googlecode.com";
    Log(LOG_TERMINAL) << "";
    Log(LOG_TERMINAL) << "This bot does have Epic Powers.";
    Log(LOG_TERMINAL) << "Type " << dir << " --help for help on how to use ANT, or read the readme.";
    exit(0);
  }

  if(args.HasArg("protocoldebug", 'p'))
  {
    protocoldebug = true;
    Log(LOG_RAWIO) << "ANT Commit System is started in Protocol Debug mode.";
  }

  if(args.HasArg("nocolor", 'c'))
  {
    nocolor = true;
    Log() << "ANT Commit System is started in No Colors mode.";
  }

  // Set terminal colors to whatever is in the config
  if(!nocolor)
    Log(LOG_TERMINAL) << Config->LogColor;

  ModuleHandler::SanitizeRuntime();
  LoadModules(); //load modules
  WritePID(); //Write the pid file
  FOREACH_MOD(I_OnStart, OnStart(argc, argv)); //announce we are starting the bot
  Fork(); //Fork to background
  SocketEngine::Init(); //Initialize the socket engine
}

// Clean up our pointers so we don't exit with memory leaks..
void GarbageCollect()
{
  // Set the segfault location incase we fuck up somewhere
  SET_SEGV_LOCATION();
  // Announce the shutdown
  FOREACH_MOD(I_OnShutdown, OnShutdown());
  // Save the databases
  SaveDatabases();
  // Announce garbage cleanup
  FOREACH_MOD(I_OnGarbageCleanup, OnGarbageCleanup());
  // Read/Write the last bit, close any leftover sockets
  if(Config)
    SocketEngine::Process();
  // Unload all modules
  ModuleHandler::UnloadAll();

  // Clean up any network pointers and clear the map
  for(Flux::insensitive_map<Network*>::iterator it = Networks.begin(), it_end = Networks.end(); it != it_end;)
  {
    Network *n = it->second;
    ++it;
    n->Disconnect("Shutting down.");

    // Clean up any channel pointers for the network and clear the map
    if(!n->ChanMap.empty())
      for(Flux::insensitive_map<Channel*>::iterator cit = n->ChanMap.begin(),
	cit_end = n->ChanMap.end(); cit != cit_end; ++cit)
      {
	Channel *c = cit->second;
	++cit;
	DeleteZero(c);
      }

    // Clean up any user pointers for the network and clear the map
    if(!n->UserNickList.empty())
      for(Flux::insensitive_map<User*>::iterator uit = n->UserNickList.begin(),
	uit_end = n->UserNickList.end(); uit != uit_end;)
      {
	User *u = uit->second;
	++uit;
	DeleteZero(u);
      }

      n->ChanMap.clear();
      n->UserNickList.clear();
      DeleteZero(n);
  }
  Networks.clear();

  // Delete our global IRC protocol wrapper
  if(GProto)
    DeleteZero(GProto);

  // Shutdown the socket engine and close any remaining sockets.
  if(Config)
    SocketEngine::Shutdown();
  ModuleHandler::SanitizeRuntime();

  // oh noes! lost pointers!
  for(std::vector<Base*>::iterator it = BaseReferences.begin(), it_end = BaseReferences.end(); it != it_end; )
  {
    Base *b = *it;
    ++it;
    Log(LOG_MEMORY) << "Deleting lost pointer: @" << b;
    DeleteZero(b);
  }
  BaseReferences.clear();
  // Vectors don't always clear, so clear them here.
  std::vector<Base*>().swap(BaseReferences);
}


