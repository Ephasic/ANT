/* Arbitrary Navn Tool -- CTCP handling module
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#include "modules.h"

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

class ctcp: public Module
{
public:
  ctcp(const Flux::string &Name):Module(Name, MOD_NORMAL)
  {
    this->SetAuthor("Justasic");
    this->SetVersion(VERSION);
    this->SetPriority(PRIORITY_FIRST);
    ModuleHandler::Attach(I_OnCTCP, this);
  }
  void OnCTCP(const Flux::string &source, const std::vector<Flux::string> &params, Network *n)
  {
    Flux::string cmd = params.empty()?"":params[0];
    Log() << "\033[22;31mRecieved CTCP " << Flux::Sanitize(cmd) << " from " << source << Config->LogColor;

    if(cmd == "\001VERSION\001")
    { // for CTCP VERSION reply
      struct utsname uts;
      if(uname(&uts) < 0)
	      throw CoreException("uname() Error");

	n->b->ircproto->notice(source, "\001VERSION ANT-%s %s %s\001",VERSION_FULL, uts.sysname, uts.machine);
    }

    if(cmd == "\001TIME\001")
	n->b->ircproto->notice(source,"\001TIME %s\001", do_strftime(time(NULL), true).c_str());

    if(cmd == "\001SOURCE\001")
      n->b->ircproto->notice(source, "\001SOURCE http://code.google.com/p/arbitrary-navn-tool/\001");

    if(cmd == "\001DCC")
      n->b->ircproto->notice(source, "I do not accept or support DCC connections.");
  }
};
/**
 * @}
 */
MODULE_HOOK(ctcp)