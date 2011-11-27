/* All code is licensed under GNU General Public License GPL v3 (http://www.gnu.org/licenses/gpl.html) */
#include "flux_net_irc.hpp"

/**
 * \file ctcp.h Header file holding the \a ctcp functions.
 * \author Lordofsraam. Polished by Justasic.
 */

/**
 * \defgroup ctcpM CTCP Module
 * This is the module for the CTCP functions.
 * It houses the CTCP replies when someone versions it or asks for the time
 * For a better description see the function description.
 * \section commands Commands associated with this module.
 * \subsection none none
 * This module has no user interactive function
 * @{
 */

/**
 * \fn class ctcp(bool a):module("CTCP", a, PRIORITY_FIRST){ this->SetDesc("CTCP Handler"); }
 * \brief Replies to CTCPs
 * Replies to CTCP requests in IRC
 */

class ctcp:public module
{
public:
  ctcp(const Flux::string &Name):module(Name){
    this->SetAuthor("Justasic");
    this->SetVersion(VERSION);
    this->SetPriority(PRIORITY_FIRST);
    ModuleHandler::Attach(I_OnCTCP, this);
  }
  void OnCTCP(const Flux::string &source, const std::vector<Flux::string> &params)
  {
    Flux::string cmd = params.empty()?"":params[0];
    Log() << "\033[22;31mRecieved CTCP " << Flux::Sanitize(cmd) << " from " << source << "\033[22;36m";
      
    if(cmd == "\001VERSION\001"){ // for CTCP VERSION reply
      struct utsname uts;
      if(uname(&uts) < 0)
	      throw CoreException("uname() Error");

	ircproto->notice(source, "\001VERSION Navn-%s %s %s\001",VERSION_FULL, uts.sysname, uts.machine);
    }
    if(cmd == "\001TIME\001"){ // for CTCP TIME reply
	ircproto->notice(source,"\001TIME %s\001", do_strftime(time(NULL), true).c_str());
    }
    if(cmd == "\001SOURCE\001"){
      ircproto->notice(source, "\001SOURCE https://gitorious.org/navn/navn\001");
      ircproto->notice(source, "\001SOURCE http://flux-net.googlecode.com/svn/branches/Navn/\001");
      ircproto->notice(source, "\1SOURCE git://gitorious.org/navn/navn.git\1");
    }
    if(cmd == "\001DCC")
      ircproto->notice(source, "I do not accept or support DCC connections.");
  }
};
/**
 * @}
 */
MODULE_HOOK(ctcp)