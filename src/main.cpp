/* Arbitrary Navn Tool -- main source file.
 * 
 * (C) 2011-2012 Flux-Net
 * Contact us at Dev@Flux-Net.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING); if not, write to the
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
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
#include "modules.h"

int loopcount;

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
  try
  {
    startup(argcx, argvx, envp);
    time_t last_check = time(NULL);

    new DBSave(); //Start the Database Save timer.
//     GProto = new GlobalProto();
    
    TimerManager::TickTimers(time(NULL)); //Call timers to tick to start sockets instantly.
    while(!quitting)
    {
      Log(LOG_RAWIO) << "Top of main loop";
      //prevent loop bombs, we raise a segfault because the segfault handler will handle it better
      if(++loopcount >= 50) { LastBuf = "50 main loop calls in 3 secs"; raise(SIGSEGV); }

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
    SaveDatabases();
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

