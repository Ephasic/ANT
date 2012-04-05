/* Arbitrary Navn Tool -- IRC System Module
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

class CommandRehash : public Command
{
public:
  CommandRehash(module *m):Command(m, "REHASH", C_PRIVATE)
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
  CommandRestart(module *m):Command(m, "RESTART", C_PRIVATE, 0, 1)
  {
   this->SetDesc("Restarts the bot");
   this->SetSyntax("\37reason\37");
  }
  void Run(CommandSource &source, const std::vector<Flux::string> &params)
  {
    //if(source.u->IsOwner()){
      restart("Restarting..");
      Log(source.u, this) << " to restart the bot.";
    //}else
      //source.Reply(ACCESS_DENIED);
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
  CommandQuit(module *m):Command(m, "QUIT", C_PRIVATE, 1, 1)
  {
    this->SetDesc("Quits the bot from IRC");
    this->SetSyntax("\37randompass\37");
  }
  void Run(CommandSource &source, const std::vector<Flux::string> &params)
  {
    User *u = source.u;
    source.Reply("Quitting..");
    Log(u) << "quit the bot from network: \"" << source.n->name << "\"";
    source.n->Disconnect(printfify("Requested From \2%s\17.", u->nick.c_str()));
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
  CommandPID(module *m):Command(m, "PID", C_PRIVATE)
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

// This void sends some random junk to the server when its called
// by a tqueue.
void SendJunk()
{
  for(auto it : Networks)
  {
    Network *n = it.second;
    if(n->s && n->s->GetStatus(SF_CONNECTED))
      n->s->Write("randomjunkcheck");
  }
}

class m_system : public module
{
  CommandRehash cmdrehash;
  CommandQuit cmdquit;
  CommandRestart cmdrestart;
  CommandPID pid;

public:
  m_system(const Flux::string &Name):module(Name), cmdrehash(this), cmdquit(this), cmdrestart(this), pid(this)
  {
    Implementation i[] = { I_OnNumeric, I_OnKick, I_OnNotice, I_OnNickChange, I_OnPrivmsg };
    ModuleHandler::Attach(i, this, sizeof(i)/sizeof(Implementation));

    this->SetAuthor("Justasic");
    this->SetVersion(VERSION);
  }

  void OnNumeric(int i, Network *n, const Flux::vector &params)
  {
    if((i == 4))
    {
      /* Numeric 004
       * params[0] = Bots nickname
       * params[1] = servername
       * params[2] = ircd version
       * params[3] = user modes
       * params[4] = channel modes
       */

      RenameBot(n, params[0]);

      if(params[3].search('B'))
	n->b->SetMode("+B"); //FIXME: get bot mode?
      if(params[2].search_ci("ircd-seven") && params[3].search('Q'))
	n->b->SetMode("+Q"); //for freenode to stop those redirects

      sepstream cs(Config->Channel, ',');
      Flux::string tok;
      while(cs.GetToken(tok))
      {
	tok.trim();
	new Channel(n, tok);
      }

      n->servername = params[1];
      n->ircdversion = params[2];
      JoinChansInBuffer(n);
    }

    // Send Random garbage occasionally to see if we need to rename
    if((i == 421))
    {
      RenameBot(n, params[0]);
    }

    /* Nickname is in use numeric
     * Numeric 433
     * params[0] = our current nickname
     * params[1] = Attempted nickname
     * params[2] = message (useless)
     */
    if((i == 433))
    {
      //int derp;
      // FIXME: Check internally for the nickname and start from there so we don't waste bandwidth
	RenameBot(n, params[1]);
    }
  }

  // user!ident@host.com NICK NewNickname
  // User*			msg
  void OnNickChange(User *u, const Flux::string &msg)
  {
    Log(LOG_TERMINAL) << "Rename: " << u->nick << " " << msg << " " << u->n->b->nick;

    if(u->nick.search(Config->NicknamePrefix))
    {
      Log(LOG_TERMINAL) << "Is bot nickname!";
//     :Server.test.net 433 ANT-1 ANT-2 :Nickname is already in use.
      RenameBot(u->n, msg);
      new tqueue(SendJunk, 10);
    }
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