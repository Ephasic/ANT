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
      if(n->b->ircproto)
	n->b->ircproto->quit("Restarting: No Reason");
    } else {
      Log() << "Restarting: " << reason;
      if(n->b->ircproto)
	n->b->ircproto->quit("Restarting: %s", reason.c_str());
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
    const Flux::string bi_dir = Config->Binary_Dir;
    BotConfig *configtmp = Config;
    Config = new BotConfig(bi_dir);
    delete configtmp;
    if(!Config)
      throw ConfigException("Could not read config.");
    FOREACH_MOD(I_OnReload, OnReload());
    ReadConfig();
  }catch(const ConfigException &ex){
    Log() << "Configuration Exception Caught: " << ex.GetReason();
  }
}

/**
 * \fn static void remove_pidfile()
 * \brief Removes the PID file on exit
 */
static void remove_pidfile() { Delete(Config->PidFile.c_str()); }
/**
 * \fn static void WritePID()
 * \brief Write the bots PID file
 */
static void WritePID(){
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
/**This is the startup sequence that starts at the top to the try loop
 * @param startup(int, char)
 */
void startup(int argc, char** argv, char *envp[])
{
  SET_SEGV_LOCATION();
  InitSignals();
  Config = NULL;
  my_av = argv;
  my_envp = envp;
  starttime = time(NULL); //for bot uptime
  
  binary_dir = getprogdir(argv[0]);
  if(binary_dir[binary_dir.length() - 1] == '.')
    binary_dir = binary_dir.substr(0, binary_dir.length() - 2);
  
  Config = new BotConfig(binary_dir);
  if(!Config)
    throw CoreException("Config Error.");
  
  Flux::string dir = argv[0];
  Flux::string::size_type n = dir.rfind('/');
  dir = "." + dir.substr(n);
  //gets the command line paramitors if any.
  if (!(argc < 1) || argv[1] != NULL)
  {
    for(int Arg=1; Arg < argc; ++Arg)
    {
      Flux::string arg = argv[Arg];
      if((arg.equals_ci("--developer")) ^ (arg.equals_ci("--dev")) ^ (arg == "-d"))
      {
	dev = nofork = true;
	Log(LOG_DEBUG) << "ANT Commit System is started in Developer mode. (" << arg << ")";
      }
      else if ((arg.equals_ci("--nofork")) ^ (arg == "-n"))
      {
	nofork = true;
	Log(LOG_DEBUG) << "ANT Commit System is started With No Forking enabled. (" << arg << ")";
      }
      else if ((arg.equals_ci("--help")) ^ (arg == "-h"))
      {
	Log(LOG_TERMINAL) << "ANT Internet Relay Chat Commit Bot system v" << VERSION;
	Log(LOG_TERMINAL) << "Usage: " << dir << " [options]";
	Log(LOG_TERMINAL) << "-h, --help";
	Log(LOG_TERMINAL) << "-d, --developer";
	Log(LOG_TERMINAL) << "-n, --nofork";
	Log(LOG_TERMINAL) << "-p, --protocoldebug";
	Log(LOG_TERMINAL) << "-c, --nocolor";
	Log(LOG_TERMINAL) << "-v, --version";
	Log(LOG_TERMINAL) << "See --version for full version information";
	Log(LOG_TERMINAL) << "This bot does have Epic Powers.";
	exit(0);
      }
      else if ((arg.equals_ci("--version")) ^ (arg == "-v"))
      {
	Log(LOG_TERMINAL) << "Arbitrary Navn Tool IRC Commit System C++ Bot Version " << VERSION_FULL;
	Log(LOG_TERMINAL) << "This bot was programmed from scratch by Justasic and Lordofsraam.";
	Log(LOG_TERMINAL) << "";
	Log(LOG_TERMINAL) << "IRC: IRC.Flux-Net.net #Computers";
	Log(LOG_TERMINAL) << "WWW: http://www.Flux-Net.net";
	Log(LOG_TERMINAL) << "Email: Staff@Flux-Net.net";
	Log(LOG_TERMINAL) << "Git: git://gitorious.org:navn/navn.git";
	Log(LOG_TERMINAL) << "";
	Log(LOG_TERMINAL) << "This bot does have Epic Powers.";
	Log(LOG_TERMINAL) << "Type " << dir << " --help for help on how to use navn, or read the readme.";
	exit(0);
      }
      else if((arg.equals_ci("--protocoldebug")) ^ (arg == "-p"))
      {
	protocoldebug = true;
	Log(LOG_RAWIO) << "ANT Commit System is started in Protocol Debug mode. (" << arg << ")";
      }
      else if((arg.equals_ci("--nocolor")) ^ (arg == "-c"))
      {
	nocolor = true;
	Log() << "ANT Commit System is started in No Colors mode. (" << arg << ")\033[0m"; //reset terminal colors
      }
      else
      {
	Log(LOG_TERMINAL) << "Unknown option " << arg;
	exit(0);
      }
    }
  }
  
  if(!nocolor)
    Log(LOG_TERMINAL) << "\033[22;36m";
  
  ModuleHandler::SanitizeRuntime();
  ReadConfig(); //load modules
  WritePID(); //Write the pid file
  FOREACH_MOD(I_OnStart, OnStart(argc, argv)); //announce we are starting the bot
  Fork(); //Fork to background
  SocketEngine::Init(); //Initialize the socket engine
}