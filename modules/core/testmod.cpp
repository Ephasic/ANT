/* Arbitrary Navn Tool -- Example Module
 * 
 * (C) 2011-2012 Flux-Net
 * Contact us at Dev@Flux-Net.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

#include "flux_net_irc.hpp"

/**
 * \example dummy.cpp
 * This is an example module showing a simple if-said-that-reply-this situation.
 * \file dummy.cpp Source file holding the \a dummy function.
 * \author Lordofsraam
 */

/**
 * \defgroup dummyM Dummy (Example) Module
 * This is a template or example module.
 * \section commands Commands associated with this module.
 * \subsection test testing testing
 * Say \a test to see if the bot replies :P
 * @{
 */

/**
 * \fn class commanddummy() : Command("TEST", 0, 0)
 * \brief Replies to a test
 * We will try to put as many examples here in the future as we can.
 */
class commanddummy : public Command
{
public:
  commanddummy() : Command("TEST", 0,1) //The 0's are how many params the command gets, they're not required and can be removed.
  {
   this->SetDesc("Test for the modules");
   this->SetSyntax("\37TEST\37");
  }
  void Run(CommandSource &source, const std::vector<Flux::string> &params)
  {
    source.Reply("YAY!");
  }
  bool OnHelp(CommandSource &source, const Flux::string &nill)
  {
    this->SendSyntax(source);
    source.Reply(" ");
    source.Reply("This command simply emmits 'YAY!' to the user.\n"
		 "This command is mostly only used to test other core\n"
		 "functions or modules in the bot");
    return true;
  }
};
class dummy : public module
{
  commanddummy cmddmy; //Declare our command
public:
  dummy(const Flux::string &Name):module(Name)
  { 
    this->AddCommand(&cmddmy); //Add our command to teh bot
    this->SetAuthor("Lordofsraam"); //Set the author
    this->SetVersion(VERSION);
    this->SetPriority(PRIORITY_LAST);
    
    //Implementation i[] = {  }; //Add that we have a module hook, this can be done in 2 ways
    ModuleHandler::Attach(I_OnPrivmsg, this);
    /*or you can do the easy way
     * ModuleHandler::Attach(i, this, sizeof(i)/sizeof(Implementation));
     */
  }
  void OnPrivmsg(User *u, Channel *c, const std::vector<Flux::string> &params)
  {
    Flux::string s;
    for(unsigned i=0; i < params.size(); ++i)
      s = params[i]+' ';
    s.trim();
    if(s.search_ci("Blah Blah"))
      u->SendMessage("Derp DERp");
   if(params[0].equals_ci("!userlist"))
   {
     Flux::string users;
     for(auto it : u->n->UserNickList)
      users += it.second->nick+" ";
    u->SendMessage(users);
   }
  }
};

MODULE_HOOK(dummy)
/**
 * @}
 */