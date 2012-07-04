/* Arbitrary Navn Tool -- Main IRC Parsing Routines
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

// Abandon all hope, ye who enter here!
// below here be scary parsing related things!

#include "user.h"
#include "bot.h"
#include "module.h"

/**
 * \fn void ProcessJoin(CommandSource &source, const Flux::string &chan)
 * \brief Processes the /who numeric (352), this should only be used in Process() unless its for something special
 * \param source CommandSource struct used to find all the information needed to make new users
 * \param chan The channel we're processing
 */

// EVENT_HOOK(command, "PING", I_OnPing, OnPing);
#define EVENT_HOOK(w, x, y, z) \
if(true) \
{ \
  if(w.equals_ci(x)) \
  {\
    FOREACH_MOD(y, z); \
  } \
} \
else \
  static_cast<void>(0);

void ProcessJoin(CommandSource &source, const Flux::string &chan)
{
    Flux::vector &params = source.params;
    if(params.size() < 8)
      return;
    
    Flux::string channel = params[1];
    Flux::string Ident = params[2];
    Flux::string Host = params[3];
    Flux::string Server = params[4];
    Flux::string Nickname = params[5];
    Flux::string flags = params[6];
    Flux::string hops = params[7].substr(0,1);
    Flux::string realname = params[7].erase(0,2);
    /*******************************************************/
    User *u = FindUser(source.n, Nickname);
    if(!u)
    {
      if((!Host.empty() || !Nickname.empty() || !Ident.empty()) && source.n)
	u = new User(source.n, Nickname, Ident, Host, realname, Server);
    }

    Channel *c = FindChannel(source.n, channel);
    if(!c)
    {
      if(!channel.empty() && source.n)
       c = new Channel(source.n, channel);
    }
    if(u)
      u->AddChan(c);
    if(c)
      c->AddUser(u);
}
/*********************************************************************************/
/**
 * \fn void ProcessCommand(CommandSource &Source, std::vector<Flux::string> &params2, const Flux::string &receiver, const Flux::string &command)
 * \brief Processes the Command class commands, this should only be used in Process() unless its for something special
 * \param CommandSource CommandSource struct used to find all the information needed to make new users
 * \param std::vector vector of the command string sent to the bot
 * \param Flux::string receiver
 * \param Flux::string Command sent to the bot in raw form
 */
void ProcessCommand(CommandSource &Source, Flux::vector &params2, const Flux::string &receiver, const Flux::string &command)
{
  SET_SEGV_LOCATION();
  User *u = Source.u;
  Channel *c = Source.c;
  if(!command.is_pos_number_only())
  {
    FOREACH_MOD(I_OnCommand, OnCommand(command, params2));
  }

 if(!FindCommand(params2[0], C_PRIVATE) && command == "PRIVMSG")
  {
    if(!protocoldebug)
      Log(LOG_TERMINAL) << '<' << u->nick << '-' << receiver << "> " << Source.params[1];

    if(!Source.n->IsValidChannel(receiver))
    {
      Source.Reply("Unknown command \2%s\2", Flux::Sanitize(params2[0]).c_str());
      FOREACH_MOD(I_OnPrivmsg, OnPrivmsg(u, params2));
    }
    else
    {
      Command *ccom = FindCommand(params2[0], C_CHANNEL);
      if(ccom)
      {
	Source.command = ccom->name;
	params2.erase(params2.begin());

	while(ccom->MaxParams > 0 && params2.size() > ccom->MaxParams)
	{
	  params2[ccom->MaxParams - 1] += " " + params2[ccom->MaxParams];
	  params2.erase(params2.begin() + ccom->MaxParams);
	}

	if(params2.size() < ccom->MinParams)
	{
	  ccom->OnSyntaxError(Source, !params2.empty() ? params2[params2.size() - 1] : "");
	  return;
	}
#ifdef HAVE_SETJMP_H
	if(setjmp(sigbuf) == 0)
	{
#endif
	  LastRunModule = ccom->mod;
	  ccom->Run(Source, params2);
#ifdef HAVE_SETJMP_H
	}
	else
	{
	  Log() << "Command " << ccom->name << " failed to execute. Stack Restored.";
	  Source.Reply("An internal error has occured, please contact the bots administrator in Azuru");
	}
#endif
	LastRunModule = nullptr;
      }
      else
      {
	FOREACH_MOD(I_OnChanmsg, OnChanmsg(u, c, params2)); //This will one day be a actual function for channel only messages..
      }
    }
  }
  else
  {
    Command *com = FindCommand(params2[0], C_PRIVATE);
    if(com && !Source.n->IsValidChannel(receiver) && command == "PRIVMSG")
    {
      Source.command = com->name;
      params2.erase(params2.begin());

      while(com->MaxParams > 0 && params2.size() > com->MaxParams)
      {
	 params2[com->MaxParams - 1] += " " + params2[com->MaxParams];
	 params2.erase(params2.begin() + com->MaxParams);
      }

      if(params2.size() < com->MinParams)
      {
	com->OnSyntaxError(Source, !params2.empty() ? params2[params2.size() - 1] : "");
	return;
      }
#ifdef HAVE_SETJMP_H
	if(setjmp(sigbuf) == 0)
	{
#endif
	  LastRunModule = com->mod;
	  com->Run(Source, params2);
#ifdef HAVE_SETJMP_H
	}
	else
	{
	  Log() << "Command " << com->name << " failed to execute. Stack Restored.";
	  Source.Reply("An internal error has occured, please contact the administrator in Azuru");
	}
#endif
	LastRunModule = nullptr;
    }
    else
    {
      if(!protocoldebug)
	Log(LOG_DEBUG) << Flux::Sanitize(Source.raw); //This receives ALL server commands sent to the bot..
    }
  }
}

/*********************************************************************************/

/**
 * \fn void process(const Flux::string &buffer)
 * \brief Main Processing function
 * \param buffer The raw socket buffer
 */
void process(Network *n, const Flux::string &buffer)
{

  EventResult e;
  FOREACH_RESULT(I_OnPreReceiveMessage, OnPreReceiveMessage(buffer), e);
  if(e != EVENT_CONTINUE)
    return;

  SET_SEGV_LOCATION();
  Flux::string buf = buffer;
  buf = buf.replace_all_cs("  ", " ");

  if(buf.empty())
    return;

  Flux::string source;
  if(buf[0] == ':')
  {
   size_t space = buf.find_first_of(" ");

   if(space == Flux::string::npos)
     return;

   source = buf.substr(1, space - 1);
   buf = buf.substr(space + 1);

   if(source.empty() || buf.empty())
     return;
  }

  sepstream bufferseparator(buf, ' ');
  Flux::string bufferseparator_token;
  Flux::string command = buf;

  if(bufferseparator.GetToken(bufferseparator_token))
    command = bufferseparator_token;

  std::vector<Flux::string> params;
  while(bufferseparator.GetToken(bufferseparator_token))
  {
    if(bufferseparator_token[0] == ':')
    {
      if(!bufferseparator.StreamEnd())
	params.push_back(bufferseparator_token.substr(1)+" "+bufferseparator.GetRemaining());
      else
	params.push_back(bufferseparator_token.substr(1));
      break;
    }
    else
      params.push_back(bufferseparator_token);
  }

  if(protocoldebug)
  {
    Log(LOG_TERMINAL) << "Source: " << (source.empty()?"No Source":source);
    Log(LOG_TERMINAL) << (command.is_number_only()?"Numeric":"Command") << ": " << command;

    if(params.empty())
     Log(LOG_TERMINAL) << "No Params";
   else
     for(unsigned i =0; i < params.size(); ++i)
       Log(LOG_TERMINAL) << "Params " << i << ": " << Flux::Sanitize(params[i]);
  }

  if(!n)
  {
    Log(LOG_TERMINAL) << "Process() called with no source Network??";
    return;
  }
  /***********************************************/
  /* make local variables instead of global ones */
  const Flux::string &receiver = params.size() > 0 ? params[0] : "";
  Flux::string message = params.size() > 1? params[1] : "";
  IsoHost h(source);
  Flux::string nickname = h.nick;
  Flux::string uident = h.ident;
  Flux::string uhost = h.host;
  Flux::string cmd;
  User *u = FindUser(n, nickname);
  Channel *c = FindChannel(n, receiver);
  Bot *b = n->b;
  Flux::vector params2 = ParamitizeString(message, ' ');
//   for(unsigned i = 0; i < params2.size(); ++i)
//     Log(LOG_TERMINAL) << "Params2[" << i << "]: " << Flux::Sanitize(params2[i]);
  /***********************************************/

  if(command == "004" && source.search('.'))
  {
    FOREACH_MOD(I_OnUserRegister, OnUserRegister(n));
  }

  if(message[0] == '\1' && message[message.length() -1] == '\1' && !params2[0].equals_cs("\001ACTION"))
  {
    //Dont allow the rest of the system to process ctcp's as it will be caught by the command handler.
    FOREACH_MOD(I_OnCTCP, OnCTCP(nickname, params2, n));
    return;
  }
  // Handle Actions (ie. /me's )
  else if(message[0] == '\1' && message[message.length() - 1] == '\1' && params2[0].equals_ci("\001ACTION"))
  {
    if(c)
    {
      FOREACH_MOD(I_OnChannelAction, OnChannelAction(u, c, params2));
    }
    else
    {
      FOREACH_MOD(I_OnAction, OnAction(u, params2));
    }
  }

  if(command.equals_cs("NICK") && u)
  {
    FOREACH_MOD(I_OnPreNickChange, OnPreNickChange(u, params[0]));
    u->SetNewNick(params[0]);
    FOREACH_MOD(I_OnNickChange, OnNickChange(u));
  }

  if(!u && !FindUser(n, nickname) && (!nickname.empty() || !uident.empty() || !uhost.empty()))
  {
    if(!nickname.search('.') && n)
      u = new User(n, nickname, uident, uhost);
  }

  if(command.equals_ci("QUIT"))
  {
    FOREACH_MOD(I_OnQuit, OnQuit(u, params[0]));
    QuitUser(n, u);
  }

  if(command.equals_ci("PART"))
  {
    FOREACH_MOD(I_OnPart, OnPart(u, c, params[0]));
    if(n->IsValidChannel(receiver) && c && u && u == n->b)
     delete c; //This should remove the channel from all users if the bot is parting..
    else
    {
     if(u)
       u->DelChan(c);

     if(c)
       c->DelUser(u);

     if(u && c && !u->findchannel(c->name))
     {
       Log(LOG_TERMINAL) << "Deleted " << u->nick << '|' << c->name << '|' << u->findchannel(c->name);
       delete u;
     }
    }
  }

  if(command.is_pos_number_only()) { FOREACH_MOD(I_OnNumeric, OnNumeric((int)command, n, params)); }

  EVENT_HOOK(command, "PING", I_OnPing, OnPing(params, n));
  EVENT_HOOK(command, "PONG", I_OnPong, OnPong(params, n));
  EVENT_HOOK(command, "KICK", I_OnKick, OnKick(u, FindUser(n, params[1]), FindChannel(n, params[0]), params[2]));
  EVENT_HOOK(command, "ERROR", I_OnConnectionError, OnConnectionError(buffer));
  EVENT_HOOK(command, "INVITE", I_OnInvite, OnInvite(u, params[1]));

  if(command.equals_ci("NOTICE") && !source.find('.'))
  {
    if(!n->IsValidChannel(receiver))
    {
      FOREACH_MOD(I_OnNotice, OnNotice(u, params2));
    }
    else
    {
      FOREACH_MOD(I_OnNotice, OnChannelNotice(u, c, params2));
    }
  }

  if(command.equals_ci("MODE"))
  {
    if(n->IsValidChannel(params[0]) && params.size() == 2)
    {
      FOREACH_MOD(I_OnChannelMode, OnChannelMode(u, c, params[1]));
    }

    else if(n->IsValidChannel(params[0]) && params.size() == 3)
    {
      FOREACH_MOD(I_OnChannelOp, OnChannelOp(u, c, params[1], params[2]));
    }

    else if(params[0] == n->b->nick)
    {
      FOREACH_MOD(I_OnUserMode, OnUserMode(u, params[0], params[1]));
    }
  }

  if(command.equals_ci("JOIN"))
  {
    if(!u && n && (!nickname.empty() || !uident.empty() || !uhost.empty()))
      u = new User(n, nickname, uident, uhost);
    else if(!c && n && n->IsValidChannel(receiver))
      c = new Channel(n, receiver);
    else if(!u->findchannel(c->name))
      u->AddChan(c);
    else if(!c->finduser(n, u->nick))
      c->AddUser(u);
    else if(u != n->b)
    {
      FOREACH_MOD(I_OnJoin, OnJoin(u, c));
    }
  }
  /**************************************/
  CommandSource Source;
  Source.u = u; //User class
  Source.b = b; //Bot class
  Source.c = c; //Channel class
  Source.n = n; //Network class
  Source.params = params;
  Source.raw = buffer;
  /**************************************/
  if(command == "352")
    ProcessJoin(Source, c->name);

  if(source.empty() || message.empty() || params2.empty())
    return;

  ProcessCommand(Source, params2, receiver, command);
  command.clear();
}