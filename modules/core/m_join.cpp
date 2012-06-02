/* Arbitrary Navn Tool -- IRC Join Management Module
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

class CommandJoin : public Command
{
public:
  CommandJoin(Module *m) : Command(m, "JOIN", C_PRIVATE, 1, 1)
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
      Log(u) << "made " << source.n->b->nick << " join " << chan << " on network " << source.n->name;
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
  CommandPart(Module *m):Command(m, "PART", C_PRIVATE, 1,1)
  {
    this->SetDesc("Part a channel");
    this->SetSyntax("\37channel\37");
  }
  void Run(CommandSource &source, const Flux::vector &params)
  {
    Flux::string chan = params[1];
    if(chan.empty())
      return;
    
    User *u = source.u;
    if(!IsValidChannel(chan))
     source.Reply(CHANNEL_X_INVALID, chan.c_str());
    else
    {
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

class CommandConnect : public Command
{
public:
  CommandConnect(Module *m):Command(m, "CONNECT", C_PRIVATE, 2,4)
  {
    this->SetDesc("Connect to a network");
    this->SetSyntax("\37networkname\37 hostname [port]");
  }
  
  void Run(CommandSource &source, const Flux::vector &params)
  {
    Flux::string NetworkName = params[0];
    Flux::string NetworkHost = params[1];
    Flux::string NetworkPort = params.size() == 3 ? params[3] : "6667";
    Network *n = FindNetwork(NetworkName);

    if(n)
    {
      source.Reply("\2%s\2 is already connected to \2%s\2", n->b->nick.c_str(), n->name.c_str());
      return;
    }
    else
    {
      try
      {
	n = new Network(NetworkHost, NetworkPort, NetworkName);
	n->Connect();
	source.Reply("Scheduled a connection to \2%s\2 (%s:%s)", NetworkName.c_str(), NetworkHost.c_str(), NetworkPort.c_str());
	Log(source.u, this) << "to connect to " << NetworkName << " (" << NetworkHost << ':' << NetworkPort << ')';
      }
      catch(const SocketException &e)
      {
	source.Reply("Cannot connect to %s: %s", NetworkName.c_str(), e.GetReason());
	Log(source.u, this) << "which received a socket exception: " << e.GetReason();
      }
    }
  }
  
  bool OnHelp(CommandSource &source, const Flux::string &nill)
  {
    this->SendSyntax(source);
    source.Reply(" ");
    source.Reply("Connect to a specified network to announce\n"
      "commits for, once the network is connected you must manually\n"
      "join channels, after a database save, the daemon will automatically\n"
      "reconnect to the network and it's channels on every start with that\n"
      "database.");
    return true;
  }
};

class CommandDisconnect : public Command
{
public:
  CommandDisconnect(Module *m):Command(m, "DISCONNECT", C_PRIVATE, 1,1)
  {
    this->SetDesc("Disconnect a network");
    this->SetSyntax("\37networkname\37");
  }
  
  void Run(CommandSource &source, const Flux::vector &params)
  {
    Network *n = FindNetwork(params[0]);
    
    if(n)
    {
      source.Reply("Disconnecting from \2%s\2", n->name.c_str());
      delete n;
    }
    else
      source.Reply("\2%s\2 does not exist.", params[0].c_str());
  }
  
  bool OnHelp(CommandSource &source, const Flux::string &nill)
  {
    this->SendSyntax(source);
    source.Reply(" ");
    source.Reply("Disconnect from an already connected network, this\n"
      "will remove any database instances once a database is saved\n");
    return true;
  }
};

class m_Join : public Module
{
  CommandJoin cmdjoin;
  CommandPart cmdpart;
  CommandConnect cmdconnect;
  CommandDisconnect cmddisconnect;
public:
  m_Join(const Flux::string &Name):Module(Name, MOD_NORMAL), cmdjoin(this), cmdpart(this), cmdconnect(this), cmddisconnect(this)
  {
    this->SetVersion(VERSION);
    this->SetAuthor("Justasic");
  }
};

MODULE_HOOK(m_Join)