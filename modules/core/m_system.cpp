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
    User *u = source.u;
   if(!u->IsOwner())
      source.Reply(ACCESS_DENIED);
   else{
     source.Reply("Rehashing Configuration file");
     Log(u, this) << "to rehash the configuration";
     Rehash();
   }
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

class CommandNick : public Command
{
public:
  CommandNick():Command("NICK", 1, 1)
  {
    this->SetDesc("Change the bots nickname");
    this->SetSyntax("\37nickname\37");
  }
  void Run(CommandSource &source, const std::vector<Flux::string> &params)
  {
    User *u = source.u;
    Flux::string newnick = params[1];
    if(newnick.empty()){
     this->SendSyntax(source);
     return;
    }
    if(!u->IsOwner()){
     source.Reply(ACCESS_DENIED);
     return;
    }
    for(unsigned i = 0, end = newnick.length(); i < end; i++)
      if(!isvalidnick(newnick[i])){
	source.Reply("\2%s\2 is an invalid nickname.");
	Config->BotNick = newnick;
      }
      source.n->ircproto->nick(newnick);
    Log(u, this) << "to change the bots nickname to " << newnick;
  }
  bool OnHelp(CommandSource &source, const Flux::string &nill)
  {
    this->SendSyntax(source);
    source.Reply(" ");
    source.Reply("This command changes the bots nickname\n"
		 "in IRC to a valid IRC nickname that is\n"
		 "checked before being set.\n"
		 "Note: You must be the bots owner to use this command");
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

class CommandKick : public Command
{
public:
  CommandKick():Command("KICK", 1, 3)
  {
    this->SetDesc("Kick a user from the channel");
    this->SetSyntax("channel \37nick\15");
  }
  void Run(CommandSource &source, const std::vector<Flux::string> &params)
  {
    User *u = source.u;
    if(u->IsOwner()){
      Flux::string kickchan = params[1];
      Flux::string kickee = params[2];
      if(kickee.empty() || kickchan.empty()){
	this->SendSyntax(source);
	return;
      }
      if(!IsValidChannel(kickchan)){
	source.Reply(CHANNEL_X_INVALID, kickchan.c_str()); 
	return;
      }
      Channel *c = findchannel(source.n, kickchan);
      if(!c){
	source.Reply("I am not in channel \2%s\2", kickchan.c_str());
	return;
      }
      c->kick(kickee, "Kick from %s", u->nick.c_str());
    }else{
      source.Reply(ACCESS_DENIED);
    }
  }
  bool OnHelp(CommandSource &source, const Flux::string &nill)
  {
    this->SendSyntax(source);
    source.Reply(" ");
    source.Reply("This command kicks a user from the channel\n"
		 "if the bot is an op higher than the user\n"
		 "to be kicked\n"
		 "Note: You must be a bot owner to use this command");
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
    Flux::string pass = params[1];
    
    if(pass == password || pass == Config->UserPass){
      source.Reply("Quitting..");
      Log(u) << "quit the bot with password: \"" << password << "\"";
      source.n->ircproto->quit("Requested From \2%s\17. Pass: \00320%s\017", u->nick.c_str(), password.c_str());
      quitting = true;
    }else{
      source.Reply(ACCESS_DENIED);
      Log(u) << "Attempted to quit the bot";
    }
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
    User *u = source.u;
    if(u->IsOwner()){
      source.Reply("My PID is: \2%i\2", (int)getpid());
      Log(u, this) << "command to get navn's PID " << getpid();
    }else{
      source.Reply(ACCESS_DENIED);
    }
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

class CommandPass: public Command
{
public:
  CommandPass():Command("PASS", 0,0)
  {
    this->SetDesc("Gets the bots Random Password");
  }
  void Run(CommandSource &source, const std::vector<Flux::string> &params)
  {
    User *u = source.u;
    if (u->IsOwner()){
      source.Reply("The password is:\2 %s", password.c_str());
      Log(u, this) << "to request navn's quit password: " << password;
    }else{
      source.Reply(ACCESS_DENIED);
      Log(u, this) << "to attempt to request navn's quit password.";
    }
  }
  bool OnHelp(CommandSource &source, const Flux::string &nill)
  {
    this->SendSyntax(source);
    source.Reply(" ");
    source.Reply("This command displays the bots randomly\n"
		 "generated password to the user so they may\n"
		 "quit the bot or use a command which requires\n"
		 "the random password\n"
		 "Note: You must be the bots owner to use this command");
    return true;
  }
};

class m_system : public module
{
  CommandKick cmdkick;
  CommandRehash cmdrehash;
  CommandQuit cmdquit;
  CommandRestart cmdrestart;
  CommandNick nick;
  CommandPID pid;
  CommandPass pass;
public:
  m_system(const Flux::string &Name):module(Name)
  {
    this->AddCommand(&cmdrehash);
    this->AddCommand(&cmdkick);
    this->AddCommand(&cmdquit);
    this->AddCommand(&cmdrestart);
    this->AddCommand(&nick);
    this->AddCommand(&pid);
    this->AddCommand(&pass);
    Implementation i[] = { I_OnNumeric, I_OnJoin, I_OnKick, I_OnNotice, I_OnChannelOp };
    ModuleHandler::Attach(i, this, sizeof(i)/sizeof(Implementation));
    this->SetAuthor("Justasic");
    this->SetVersion(VERSION);
  }
  void OnNumeric(int i, Network *n)
  {
    if((i == 4)){
      //ircproto->mode(Config->BotNick, "+B"); get bot mode?
      sepstream cs(Config->Channel, ',');
      Flux::string tok;
      while(cs.GetToken(tok))
      {
	tok.trim();
	Channel *c = new Channel(FluxNet, tok);
	c->SendJoin();
      }
    }
    if((i == 433)){
      Config->BotNick.push_back(Flux::RandomNickString(5));
      n->ircproto->nick(Config->BotNick);
      istempnick = true;
    }
    if((i == 376))
    {
      Log(LOG_TERMINAL) << "\033[22;31mStarted with PID \033[22;32m" << getpid() << "\033[22;36m";
      Log(LOG_TERMINAL) << "\033[22;34mSession Password: \033[01;32m" << password << "\033[22;36m";
      started = true;
      /* Identify to the networks services */
//       if((!Config->ServicesAccount.empty() || !Config->ServicesPass.empty()) && Config->IdentOnConn){
// 	Flux::string Sendns = Config->ServicesSendString.replace_all_ci("%a", Config->ServicesAccount);
// 	Sendns = Sendns.replace_all_ci("%p", Config->ServicesPass);
// 	n->ircproto->privmsg(Config->ServicesService, Sendns);
// 	Log() << "Identified to " << Config->ServicesService << " with account \"" << Config->ServicesAccount << "\"";
//       }
    }
  }
  void OnKick(User *u, User *kickee, Channel *c, const Flux::string &reason)
  {
     if(kickee && kickee->nick.equals_ci(Config->BotNick))
     {
       Log(u) << "kicked me from " << c->name << '(' << reason << ')';
	c->SendJoin(); 
     }
  }
  void OnNotice(User *u, const std::vector<Flux::string> &params)
  {
    Flux::string msg;
    for(unsigned i=0; i < params.size(); ++i)
      msg += params[i]+' ';
      
    msg.trim(); // Auto-Identify
    if(msg.search(Config->AutoIdentString)){
      if((!Config->ServicesPass.empty() || !Config->ServicesAccount.empty()) && u->nick == Config->ServicesService && Config->IdentOnConn){
	Flux::string Sendns = Config->ServicesSendString.replace_all_ci("%a", Config->ServicesAccount);
	Sendns = Sendns.replace_all_ci("%p", Config->ServicesPass);
	u->SendPrivmsg(Sendns);
	Log() << "Identified to " << u->nick << " with account \"" << Config->ServicesAccount << "\"";
      }
    }
  }
  void OnNickChange(User *u, const Flux::string &newnick)
  {
    IsoHost *Host = new IsoHost(u->fullhost);
    if(Host->nick == Config->BotNick)
      Config->BotNick = Host->nick;
    if(newnick == Config->Owner)
      Config->Owner = Host->nick;
    delete Host;
   /*if(command == "NICK"){
   if(u && u->nick == Config->BotNick){
       nick = params[0];
       delete u; //we shouldnt be a user in the 1st place (:
  }else if(u->IsOwner())
    owner_nick = params[0];
  else
    delete u; //we delete the user because the above if statement makes a new one for the nick change
   }*/
  }
};

MODULE_HOOK(m_system)