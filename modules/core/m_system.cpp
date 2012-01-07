/* Arbitrary Navn Tool -- IRC System Module
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

struct sysinfo sys_info;
class CommandRehash : public Command
{
public:
  CommandRehash():Command("REHASH", 0, 0)
  {
    this->SetDesc("Rehashes the config file");
  }
  void Run(CommandSource &source, const std::vector<Flux::string> &params)
  {
     source.Reply("Rehashing Configuration file");
     Log(source.u, this) << "to rehash the configuration";
     Rehash();
  }
  bool OnHelp(CommandSource &source, const Flux::string &nill)
  {
    this->SendSyntax(source);
    source.Reply(" ");
    source.Reply("This command rehashes the bots configuration\n"
		 "which allows for various configuration changes\n"
		 "to be made while the bot is running\n"
		 "Note: you must be the bots owner and does not\n"
		 "change all values in the config (such as IRC nicknames)");
    return true;
  }
};

class CommandRestart : public Command
{
public:
  CommandRestart():Command("RESTART", 0, 1)
  {
   this->SetDesc("Restarts the bot");
   this->SetSyntax("\37reason\37");
  }
  void Run(CommandSource &source, const std::vector<Flux::string> &params)
  {
    if(source.u->IsOwner()){
      restart("Restarting..");
      Log(source.u, this) << " to restart the bot.";
    }else
      source.Reply(ACCESS_DENIED);
  }
  bool OnHelp(CommandSource &source, const Flux::string &nill)
  {
    this->SendSyntax(source);
    source.Reply(" ");
    source.Reply("This command restarts the bot. This will\n"
		 "make the bot quit IRC, terminate its current\n"
		 "process, and start a new one with the same\n"
		 "settings as before\n"
		 "Note: You must be the bots owner to use this command");
    return true;
  }
};

class CommandQuit : public Command
{
public:
  CommandQuit():Command("QUIT", 1, 1)
  {
    this->SetDesc("Quits the bot from IRC");
    this->SetSyntax("\37randompass\37");
  }
  void Run(CommandSource &source, const std::vector<Flux::string> &params)
  {
    User *u = source.u;
    source.Reply("Quitting..");
    Log(u) << "quit the bot from network: \"" << source.n->name << "\"";
    source.n->Disconnect(fsprintf("Requested From \2%s\17.", u->nick.c_str()));
  }
  bool OnHelp(CommandSource &source, const Flux::string &nill)
  {
    this->SendSyntax(source);
    source.Reply(" ");
    source.Reply("This command instructs the bot to quit IRC\n"
		 "with the quit message being the user who quit\n"
		 "the bot and the randomly generated password\n"
		 "Note: You must be the bots owner or know the\n"
		 "configuration set password to use this command");
    return true;
  }
};

class CommandPID: public Command
{
public:
  CommandPID():Command("PID", 0,0)
  {
    this->SetDesc("Gets the bots Process ID");
  }
  void Run(CommandSource &source, const std::vector<Flux::string> &params)
  {
      source.Reply("My PID is: \2%i\2", (int)getpid());
      Log(source.u, this) << "command to get navn's PID " << getpid();
  }
  bool OnHelp(CommandSource &source, const Flux::string &nill)
  {
    this->SendSyntax(source);
    source.Reply(" ");
    source.Reply("This command returns the bots current\n"
		 "Process ID in linux or windows.\n"
		 "Note: You must be a bot owner to use this command");
    return true;
  }
};

class m_system : public module
{
  CommandRehash cmdrehash;
  CommandQuit cmdquit;
  CommandRestart cmdrestart;
  CommandPID pid;
public:
  m_system(const Flux::string &Name):module(Name)
  {
    this->AddCommand(&cmdrehash);
    this->AddCommand(&cmdquit);
    this->AddCommand(&cmdrestart);
    this->AddCommand(&pid);
    Implementation i[] = { I_OnNumeric, I_OnKick, I_OnNotice, I_OnNickChange };
    ModuleHandler::Attach(i, this, sizeof(i)/sizeof(Implementation));
    this->SetAuthor("Justasic");
    this->SetVersion(VERSION);
  }
  void OnNumeric(int i, Network *n, const Flux::vector &params)
  {
    if((i == 4)){
      /* Numeric 004
       * params[0] = Bots nickname
       * params[1] = servername
       * params[2] = ircd version
       * params[3] = user modes
       * params[4] = channel modes
       */
      if(params[3].search('B'))
	n->b->ircproto->mode(n->b->nick, "+B"); //FIXME: get bot mode?
      sepstream cs(Config->Channel, ',');
      Flux::string tok;
      while(cs.GetToken(tok))
      {
	tok.trim();
	Channel *c = new Channel(n, tok);
	c->SendJoin();
      }
      n->servername = params[1];
      n->ircdversion = params[2];
      JoinChansInBuffer(n);
    }
    /* Numeric 433
     * params[0] = *
     * params[1] = Attempted nickname
     * params[2] = message
     */
    if((i == 433))
    {
      int num = (int)params[1].substr(Config->NicknamePrefix.size());
      n->b->SetNick(Config->NicknamePrefix+value_cast<Flux::string>(++num));
    }
  }

  void OnNickChange(User *u, const Flux::string &msg)
  {
    if(u == u->n->b)
    {
      Log(LOG_TERMINAL) << u->nick << "|" << u->n->b->nick;
      int num = 0;
      if(u->nick.size() >= Config->NicknamePrefix.size())
	num = u->nick.is_pos_number_only()?(int)u->nick.substr(Config->NicknamePrefix.size()):0;
      else
	num = 0;
      if((num <= 0)){
	num++;
	Log(LOG_TERMINAL) << num;
	u->n->b->SetNick(Config->NicknamePrefix+value_cast<Flux::string>(num));
      }
    }
    u->SetNewNick(msg);
  }
  
  void OnKick(User *u, User *kickee, Channel *c, const Flux::string &reason)
  {
     if(kickee && kickee == c->n->b)
     {
       Log(u) << "kicked " << kickee->nick << " from " << c->name  << " in network " << c->n->name << ' ' << '(' << reason << ')';
	c->SendJoin();
     }
  }
};

MODULE_HOOK(m_system)