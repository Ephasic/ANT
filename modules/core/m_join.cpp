/* Arbitrary Navn Tool -- IRC Join Management Module
 * 
 * (C) 2011-2012 Azuru
 * Contact us at Dev@Flux-Net.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#include "modules.h"

/*This module setup needs serious work!
 * Justasic will work on it but its going to be hard with los's module class.
 */
class CommandJoin : public Command
{
public:
  CommandJoin(module *m) : Command(m, "JOIN", C_PRIVATE, 1, 1)
  {
   this->SetDesc("Joins a channel");
   this->SetSyntax("\37channel\37");
  }
  void Run(CommandSource &source, const std::vector<Flux::string> &params)
  {
    User *u = source.u;
    Flux::string chan = params[1];
    if(!IsValidChannel(chan))
      source.Reply(CHANNEL_X_INVALID, chan.c_str());
    else{
      Log(u) << "made the bot join " << chan << " on network " << source.n->name;
//       Channel *c = findchannel(chan);
//       if(c)
// 	c->SendJoin();
//       else
	source.n->b->ircproto->join(chan);
    }
  }
  bool OnHelp(CommandSource &source, const Flux::string &nill)
  {
    this->SendSyntax(source);
    source.Reply(" ");
    source.Reply("This command makes the bot join a channel.\n"
		 "You must be the bots owner to use this command.");
    return true;
  }
};
class CommandPart : public Command
{
public:
  CommandPart(module *m):Command(m, "PART", C_PRIVATE, 1,1)
  {
    this->SetDesc("Part a channel");
    this->SetSyntax("\37channel\37");
  }
  void Run(CommandSource &source, const std::vector<Flux::string> &params)
  {
    Flux::string chan = params[1];
    User *u = source.u;
    if(!IsValidChannel(chan))
     source.Reply(CHANNEL_X_INVALID, chan.c_str());
    else{
      Channel *c = FindChannel(source.n, chan);
      if(c)
	c->SendPart();
      else
	source.Reply("I am not in channel \2%s\2", chan.c_str());
      Log(u) << "made the bot part " << chan;
    }
  }
  bool OnHelp(CommandSource &source, const Flux::string &nill)
  {
    this->SendSyntax(source);
    source.Reply(" ");
    source.Reply("This command makes the bot part a channel.\n"
		 "You must be the bots owner to use this command.");
    return true;
  }
};
class Join : public module
{
  CommandJoin cmdjoin;
  CommandPart cmdpart;
public:
  Join(const Flux::string &Name):module(Name), cmdjoin(this), cmdpart(this)
  { 
    this->SetVersion(VERSION);
    this->SetAuthor("Justasic");
  }
};

MODULE_HOOK(Join)