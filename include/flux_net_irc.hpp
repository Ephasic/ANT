/* All code is licensed under GNU General Public License GPL v3 (http://www.gnu.org/licenses/gpl.html) */
#ifndef DERP_H
#define DERP_H
#include "module.h"
#include "defs.h"
#include "xmlfile.h"
#include "network.h" //We'll solve includes later
#include "bot.h"
IRCProto *ircproto;
SocketIO *sock;
BotConfig *Config;
module *LastRunModule; // For crashes
/**Runtime directory finder
 * This will get the bots runtime directory
 * @param getprogdir(const Flux::string dir)
 */
Flux::string getprogdir(const Flux::string &dir){
  char buffer[FILENAME_MAX];
  if (GetCurrentDir(buffer, sizeof(buffer))) {
    Flux::string remainder = dir;
    bot_bin = remainder;
    Flux::string::size_type n = bot_bin.rfind("/");
    Flux::string fullpath;
    if (bot_bin[0] == '/')
      fullpath = bot_bin.substr(0, n);
    else
      fullpath = Flux::string(buffer) + "/" + bot_bin.substr(0, n);
    bot_bin = bot_bin.substr(n + 1, remainder.length());
    return fullpath;
  }
  return "/";
}
/**
 * \fn Flux::string removeCommand(Flux::string command, Flux::string s)
 * \brief Removes a command from a Flux::string.
 * \param command String to be taken out.
 * \param s Original Flux::string.
 * This takes out \a command from \a s and returns \a s without \a command It's very useful when you want
 * to use the rest of a Flux::string as an argument for a command.
 * \return A Flux::string \a s without \a command.
 */
Flux::string removeCommand(Flux::string command, Flux::string s){
  size_t pos = s.find(command);
  return s.substr(pos+(command.size())+1);
}

/**
 * \fn Flux::string urlify(Flux::string raw_searchstring)
 * \brief Replaces special chars in a Flux::string with url compliant codes.
 * \param raw_searchstring
 * Goes through each character in a Flux::string and if it finds a special character,
 * it replaces it with what would be in a url for that character.
 * \return A Flux::string without any special characters other than %
 */
Flux::string urlify(const Flux::string &received){
  Flux::string string;
  for(unsigned i=0; i < received.size(); ++i){
    switch(received[i]){
      case '%': string = string+"%25"; break;
      case ' ': string = string+"%20"; break;
      case '+': string = string+"%2B"; break;
      case '$': string = string+"%24"; break;
      case '&': string = string+"%26"; break;
      case ',': string = string+"%2C"; break;
      case '/': string = string+"%2F"; break;
      case ':': string = string+"%3A"; break;
      case ';': string = string+"%3B"; break;
      case '=': string = string+"%3D"; break;
      case '?': string = string+"%3F"; break;
      case '@': string = string+"%40"; break;
      case '#': string = string+"%23"; break;
      case '>': string = string+"%3E"; break;
      case '<': string = string+"%3C"; break;
      case '{': string = string+"%7B"; break;
      case '}': string = string+"%7D"; break;
      case '|': string = string+"%7C"; break;
      case '\\':string = string+"%5C"; break;
      case '^': string = string+"%5E"; break;
      case '~': string = string+"%7E"; break;
      case '[': string = string+"%5B"; break;
      case ']': string = string+"%5D"; break;
      case '`': string = string+"%60"; break;
      case '*': string = string+"%2A"; break;
      case '(': string = string+"%28"; break;
      case ')': string = string+"%29"; break;
      case '\'':string = string+"%27"; break;
      case '"': string = string+"%22"; break;
      case '.': string = string+"%2E"; break;
      case '-': string = string+"%2D"; break;
      default:
	string = string+received[i];
    }
  }
  return string;
}
/**
 * \fn Flux::string execute(const char *cmd)
 * \brief Sends a command to the OS
 * \param cmd A command
 * \return A Flux::string containing the response from the OS.
 */
Flux::string execute(const char *cmd) {
  /*
   * Roses are red,
   * Violets are blue,
   * I read StackOverflow
   * And so do you!
   */
  FILE* pipe = popen(cmd, "r");
  if (!pipe) return "";
		     char buffer[128];
  Flux::string result = "";
  while(!feof(pipe)) {
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
void restart(const Flux::string &reason){
  char CurrentPath[FILENAME_MAX];
  GetCurrentDir(CurrentPath, sizeof(CurrentPath));
  FOREACH_MOD(I_OnRestart, OnRestart(reason));
  if(reason.empty()){
    Log() << "Restarting: No Reason";
    if(ircproto)
      ircproto->quit("Restarting: No Reason");
  }else{
    Log() << "Restarting: " << reason;
    if(ircproto)
      ircproto->quit("Restarting: %s", reason.c_str());
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
void Rehash(){
  Log() << "Rehashing Configuration File";
  try{
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
    ircproto->notice(Config->Owner, "Config Exception Caught: %s", ex.GetReason());
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
  if(pidfile){
    #ifdef _WIN32
    fprintf(pidfile, "%d\n", static_cast<int>(GetCurrentProcessId()));
    #else
    fprintf(pidfile, "%d\n", static_cast<int>(getpid()));
    #endif
    fclose(pidfile);
    atexit(remove_pidfile);
  }
  else
    throw CoreException("Can not write to PID file "+Config->PidFile);
}
/**This is the startup sequence that starts at the top to the try loop
 * @param startup(int, char)
 */
void startup(int argc, char** argv, char *envp[]) {
  SET_SEGV_LOCATION();
  InitSignals();
  Config = NULL;
  ircproto = NULL;
  sock = NULL;
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
  if (!(argc < 1) || argv[1] != NULL){
    for(int Arg=1; Arg < argc; ++Arg){
      Flux::string arg = argv[Arg];
      if((arg.equals_ci("--developer")) ^ (arg.equals_ci("--dev")) ^ (arg == "-d"))
      {
	dev = nofork = true;
	Log(LOG_DEBUG) << Config->BotNick << " is started in Developer mode. (" << arg << ")";
      }
      else if ((arg.equals_ci("--nofork")) ^ (arg == "-n")){
	nofork = true;
	Log(LOG_DEBUG) << Config->BotNick << " is started With No Forking enabled. (" << arg << ")";
      }
      else if ((arg.equals_ci("--help")) ^ (arg == "-h")){
	Log(LOG_TERMINAL) << "ANT Internet Relay Chat Commit Bot v" << VERSION;
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
      else if ((arg.equals_ci("--version")) ^ (arg == "-v")){
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
      else if((arg.equals_ci("--protocoldebug")) ^ (arg == "-p")){
	protocoldebug = true;
	Log(LOG_RAWIO) << Config->BotNick << " is started in Protocol Debug mode. (" << arg << ")";
      }
      else if((arg.equals_ci("--nocolor")) ^ (arg == "-c")){
	nocolor = true;
	Log() << Config->BotNick << " is started in No Colors mode. (" << arg << ")\033[0m"; //reset terminal colors
      }
      else
      {
	Log(LOG_TERMINAL) << "Unknown option " << arg;
	exit(0);
      }
    }
  }
  if(!nocolor) Log(LOG_TERMINAL) << "\033[22;36m";
  ModuleHandler::SanitizeRuntime();
  ReadConfig(); //load modules
  WritePID(); //Write the pid file
  FOREACH_MOD(I_OnStart, OnStart(argc, argv)); //announce we are starting the bot
  Fork(); //Fork to background
}

/***************************************************************************/
#endif
