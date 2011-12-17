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

int startcount, loopcount;
static void Connect()
{
  if(quitting)
    return;
  try{
    ++startcount;
    if(Config->Server.empty())
      throw SocketException("No Server Specified.");
    if(Config->Port.empty())
      throw SocketException("No Port Specified.");
      if(!FluxNet)
	FluxNet = new Network("northstar.flux-net.net", 6667, "Flux-Net");
      FluxNet->Connect();
  }catch(const SocketException &e)
  {
    Log(LOG_DEBUG) << "SocketException: " << e.GetReason();
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
    Connect(); // Connect to our 1st network
    if(++startcount >= 3) throw CoreException("Could not start master socket!");
    if(!FluxNet) goto SocketStart;
    time_t last_check = time(NULL);

    new DBSave(); //Start the Database Save timer.
//     GProto = new GlobalProto();
    
    while(!quitting)
    {
      Log(LOG_RAWIO) << "Top of main loop";
      //prevent loop bombs, we raise a segfault because the segfault handler will handle it better
      if(++loopcount >= 50) { LastBuf = "50 main loop runs in 3 secs"; raise(SIGSEGV); } 

      SocketEngine::Process();
      /* Process Timers */
      /***********************************/
      if(time(NULL) - last_check >= 3)
      {
	loopcount = 0;
	TimerManager::TickTimers(time(NULL));
	last_check = time(NULL);
      }
      /***********************************/
    } // while loop ends here
    FOREACH_MOD(I_OnShutdown, OnShutdown());
    SocketEngine::Shutdown();
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
  return EXIT_SUCCESS;
}

