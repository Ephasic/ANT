/* Arbitrary Navn Tool -- IRC help module
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
 * \file help.cpp Module file holding the \a help function.
 * \author Lordofsraam. Polished by Justasic.
 */

/**
 * \defgroup helpM Help Module
 * This is the module for the help_m function.
 * Returns help information about controlling Navn.
 * \section commands Commands associated with this module.
 * \subsection help Private message: help
 * Send a private message saying \a help to get information.
 * @{
 */

/**
 * \fn void help_m(Socket &sock, irc_string *reply)
 * \brief Returns help info on Navn.
 * Tells you most of the commands you can use on Navn.
 */
class CommandHelp : public Command
{
public:
  CommandHelp(module *m):Command(m, "HELP", C_PRIVATE, 0, 1)
  {
   this->SetDesc("Displays help messages");
  }
  void Run(CommandSource &source, const std::vector<Flux::string> &params)
  {
    int c=0;
    if(!params.empty()){
      Command *com = FindCommand(params[1], C_PRIVATE);
      if(com && !com->OnHelp(source, ""))
	source.Reply("No help available for \2%s\2", params[1].c_str());
      else if(!com)
	source.Reply("No help available for \2%s\2", params[1].c_str());
      Log(source.u) << "used help command on " << params[1];
    }else{
      for(auto val : Commandsmap)
      {
	source.Reply("\2%-16s\2 %s", val.second->name.c_str(), val.second->GetDesc().c_str());
	++c;
      }
      source.Reply("Total of \2%i\2 commands", c);
      Log(source.u) << "used help command";
    }
  }
};

class CommandCHelp : public Command
{
public:
  CommandCHelp(module *m):Command(m, "!HELP", C_CHANNEL, 0,1)
  {
   this->SetDesc("Displays Channel help messages");
  }
  void Run(CommandSource &source, const std::vector<Flux::string> &params)
  {
    Flux::string cmds;
    if(!params.empty()){
      Command *c = FindCommand(params[1], C_CHANNEL);
      if(c && !c->OnHelp(source, ""))
	source.Reply("No help available for \2%s\2", params[1].c_str());
      else if(!c)
	source.Reply("No help available for \2%s\2", params[1].c_str());
      Log(source.u) << "used help command on " << params[1];
    }else{
    for(auto val : ChanCommandMap) //As you can see C++11 is MUCH better than C++98
      cmds += val.second->name+" ";
      cmds.trim();
      cmds.tolower();
      source.c->SendMessage("Local %s Commands:", source.c->name.c_str());
      source.c->SendMessage(cmds);
      Log(source.u) << "used help command";
    }
  }
};

class help_m:public module
{
  CommandHelp help;
  CommandCHelp chelp;
public:
  help_m(const Flux::string &Name):module(Name), help(this), chelp(this){
    this->SetVersion(VERSION);
    this->SetPriority(PRIORITY_FIRST);
    this->SetAuthor("Justasic");
  }
};
MODULE_HOOK(help_m)
/**
 * @}
 */