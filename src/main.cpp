/* All code is licensed under GNU General Public License GPL v3 (http://www.gnu.org/licenses/gpl.html) */
/**
 *\file  main.cpp 
 *\brief Contains the main() function.
 *\mainpage Documentation for Navn - The C++ IRC Sockets bot by Flux-net
 *\section history Navn's History
 * Navn was started by Lordofsraam because he didn't like depending
 *  on an IRC client to run his scripts for the IRC channel. It really staterted 
 * out as a hunt to be able to tell the time in the IRC channel because
 * Justasic lived in a different timezone. From there it evolved to what you see today 
 * with the help of Justasic coming in to help with the code and debugging.
 * If you want a really detailed history go look at our svn commit log in googlecode.
 * \section dev Further Development
 * Navn is always growing and getting better. If you would like to help, contact Justasic or Lordofsraam.
 * \subsection mods Module Development
 * If you think the system stuff is too complicated, but you would still like to code for Navn to make more special
 * feature available, we have made it as easy as possible to make modules.
 * See the examples tab for an example module.
 */
#include "flux_net_irc.hpp"
FluxSocket *Fluxsocket = NULL;
FluxSocket::FluxSocket() : Socket(-1), ConnectionSocket(), BufferedSocket()
{
  Fluxsocket = this;
}
FluxSocket::~FluxSocket()
{
  ircproto->quit("Closing Socket");
  this->ProcessWrite();
  Fluxsocket = NULL;
}
bool FluxSocket::Read(const Flux::string &buf)
{
  Flux::vector params = StringVector(buf, ' '); 
  if(!params.empty() && params[0].equals_ci("ERROR"))
  {
    FOREACH_MOD(I_OnSocketError, OnSocketError(buf));
    return false; //Socket is dead so we'll let the socket engine handle it
  }
  process(buf);
  return true;
}

void FluxSocket::OnConnect()
{
  Log(LOG_TERMINAL) << "Successfuly connected to " << Config->Server << ':' << Config->Port;
  FOREACH_MOD(I_OnPostConnect, OnPostConnect(this));
  this->Write("derpy!");
  this->ProcessWrite();
}
void FluxSocket::OnError(const Flux::string &buf)
{
  Log(LOG_TERMINAL) << "Unable to connect to " << Config->Server << ':' << Config->Port << (!buf.empty()?(": " + buf):"");
}
// bool SocketIO::Read(const Flux::string &buf) const
// {
//   if(buf.search_ci("ERROR :Closing link:")){
//     FOREACH_MOD(I_OnSocketError, OnSocketError(buf));
//     throw SocketException(buf.c_str());
//   }
//   process(buf); /* Process the buffer for navn */
//   return true;
// }

int startcount, loopcount;
static void Connect()
{
  if(quitting)
    return;
  ++startcount;
  Log() << "Connecting to server '" << Config->Server << ":" << Config->Port << "'";
  if(Config->Server.empty())
    throw SocketException("No Server Specified.");
  if(Config->Port.empty())
    throw SocketException("No Port Specified.");
  new FluxSocket();
  FOREACH_MOD(I_OnPreConnect, OnPreConnect(Config->Server, Config->Port));
  Fluxsocket->Connect(Config->Server, Config->Port);
  if(ircproto){
    ircproto->user(Config->Ident, Config->Realname);
    ircproto->nick(Config->BotNick);
  }
}

class DBSave : public Timer
{
public:
  DBSave():Timer(60, time(NULL), true) {}
  void Tick(time_t)
  {
    SaveDatabases();
  }
};

int main (int argcx, char** argvx, char *envp[])
{
  SET_SEGV_LOCATION();
  startcount = 0;
  try
  {
    startup(argcx, argvx, envp);
    SocketStart:
    try { Connect(); }
    catch(SocketException &e){
      if(startcount >= 3)
	throw CoreException(e.GetReason());
      Log(LOG_DEBUG) << "Socket Exception Caught: " << e.GetReason();
      goto SocketStart;
    }
    if(!Fluxsocket)
      goto SocketStart;
    ircproto = new IRCProto(Fluxsocket);
    time_t last_check = time(NULL);

    DBSave db; //Start the Database Save timer.
    
    //Set the username and nick
    ircproto->user(Config->Ident, Config->Realname);
    ircproto->nick(Config->BotNick);
    
    while(!quitting)
    {
      Log(LOG_RAWIO) << "Top of main loop";
      if(++loopcount >= 500)
	raise(SIGSEGV); //prevent loop bombs, we raise a segfault because the segfault handler will handle it better
      
      /* Process the socket engine */
//       try { sock->Process(); }
//       catch(SocketException &exc)
//       {
// 	Log() << "Socket Exception: " << exc.description();
// 	try { Connect(); }
// 	catch(SocketException &ex){
// 	  Log() << "Socket Exception: " << ex.description();
// 	  throw CoreException(ex.description());
// 	}
	SocketEngine::Process();
	if(!Fluxsocket)
	{
	  Log(LOG_TERMINAL) << "Main count: " << loopcount;
	  try { Connect(); }
	  catch(SocketException &e){
	    Log(LOG_DEBUG) << "Socket Exception Caught: " << e.GetReason();
	  }
	}
      //}
      /* Process Timers */
      /***********************************/
      if(time(NULL) - last_check >= 3)
      {
	loopcount = 0;
	TimerManager::TickTimers(time(NULL));
	last_check = time(NULL);
      }
      /***********************************/
    }//while loop ends here
    FOREACH_MOD(I_OnShutdown, OnShutdown());
    delete Fluxsocket;
    ModuleHandler::UnloadAll();
    ModuleHandler::SanitizeRuntime();
    Log(LOG_TERMINAL) << "\033[0m";
  }//try ends here
  catch(const CoreException& e){
    /* we reset the terminal colors, this should be removed as it makes more issues than it is cool */
    Log(LOG_TERMINAL) << "\033[0m";
    if(!Config)
      Log(LOG_TERMINAL) << "Core Exception Caught: " << e.GetReason();
    else
      Log() << "Core Exception Caught: " << e.GetReason();
    return EXIT_FAILURE;
  }
  catch(std::exception &ex)
  {
    Log(LOG_TERMINAL) << "\033[0m";
    if(!Config)
      Log(LOG_TERMINAL) << "Standard Exception Caught: " << ex.what();
    else
      Log() << "Standard Exception Caught: " << ex.what();
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

