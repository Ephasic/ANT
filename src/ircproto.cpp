/* Arbitrary Navn Tool -- IRC Protocol Classes
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#include "ircproto.h"
#include "bot.h"
#include "INIReader.h"
/**
 *\file  ircproto.cpp
 *\brief Contains the IRCProto class.
 */
// Buffer all messages except critical ones and send them in a timely manner
class SendQTimer : public Timer
{
  int sent;
  const Network *n;
  struct
  {
    // The Queue of all messages, buffered;
    std::queue<Flux::string> SendQ;
    // Number of lines sent before burst
    int linessent;
    // Have we sent the burst yet?
    inline bool HasBurst() const { return (linessent <= Config->BurstRate); }
  } sqo;
  
public:
  inline bool NetworkReady() const { return (this->n && this->n->s && this->n->s->GetStatus(SF_CONNECTED)); }
  SendQTimer(const Network *net) : Timer(Config->SendQRate, time(NULL), true), sent(0), n(net)
  {
    Log(LOG_DEBUG) << "Initialized a SengQ Timer";
    sqo.linessent = 0;
  }
  
  ~SendQTimer()
  {
    Log(LOG_DEBUG) << "Clearing SendQ...";
    
    while(!this->sqo.SendQ.empty())
      this->sqo.SendQ.pop();
    
    Log(LOG_DEBUG) << "Destroying SendQ Timer";
  }

  void SendBuffered(const Flux::string &buffer)
  {
    if(Config->SendQEnabled)
    {
      if(this->sqo.HasBurst() && NetworkReady())
      {
	sqo.linessent++; // Spam for a few lines
	this->n->s->Write(buffer);
      }
      else // Oh well.. fun while it lasted lol
	sqo.SendQ.push(buffer);
    }
    else // Send Unlimited if there's no sendq enabled. Whee!
      if(NetworkReady())
	this->n->s->Write(buffer);
      else
	Log(LOG_WARN) << "Attempted to send \"" << sqo.SendQ.front() << "\" to the server but no socket exists!";
  }
  
  void Tick(time_t)
  {
    while(!sqo.SendQ.empty() && ++sent <= Config->SendQLines)
    {
      if(this->n && this->n->s && this->n->s->GetStatus(SF_CONNECTED))
	this->n->s->Write(sqo.SendQ.front());
      else
	Log(LOG_WARN) << "Attempted to send \"" << sqo.SendQ.front() << "\" to the server but no socket exists!";
      sqo.SendQ.pop();
    }
    
    if(sqo.SendQ.empty())
      sqo.linessent = 0;
    sent = 0;
  }
};

IRCProto::IRCProto(const Network *n) : net(n)
{
  if(!n)
    throw CoreException("IRCProto with no network?");
  
  this->sqt = new SendQTimer(n);
}

void IRCProto::Raw(const char *fmt, ...)
{
  va_list args;
  char buffer[BUFSIZE] = "";
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  
  if(this->net->s && this->net->s->GetStatus(SF_CONNECTED))
  {
    this->sqt->SendBuffered(Flux::string(buffer));
//     this->net->s->Write(Flux::string(buffer));
//     this->net->s->ProcessWrite();
  }
  else
    Log(LOG_WARN) << '[' << this->net->name << ']' << " Attempted to send '" << buffer << '\'';
}
/**
 * \brief Sends a IRC private message to the user or channel
 * \param Destination Where the message will go
 * \param Message The message to send to Destination
 */
void IRCProto::privmsg(const Flux::string &where, const char *fmt, ...)
{
  va_list args;
  char buffer[BUFSIZE] = "";
  if(fmt)
  {
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    this->privmsg(where, Flux::string(buffer));
    va_end(args);
  }
}
/**
 * \overload void IRCProto::privmsg(Flux::string where, Flux::string msg)
 * \brief Sends a IRC private message to the user or channel
 */
void IRCProto::privmsg(const Flux::string &where, const Flux::string &msg)
{
 sepstream sep(msg, '\n');
 Flux::string tok;
 while(sep.GetToken(tok))
   this->Raw("PRIVMSG %s :%s\n", where.c_str(), tok.c_str());
}
/**
 * \brief Sends a IRC notice to the user or channel
 * \param Destination Where the message will go
 * \param Message The message to send to Destination
 */
void IRCProto::notice(const Flux::string &where, const char *fmt, ...)
{
  va_list args;
  char buffer[BUFSIZE] = "";
  if(fmt)
  {
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    this->notice(where, Flux::string(buffer));
    va_end(args);
  }
}
/**
 * \overload void IRCProto::notice(Flux::string where, Flux::string msg)
 * \brief Sends a IRC notice to the user or channel
 */
void IRCProto::notice(const Flux::string &where, const Flux::string &msg)
{
 sepstream sep(msg, '\n');
 Flux::string tok;
 while(sep.GetToken(tok))
   this->Raw("NOTICE %s :%s\n", where.c_str(), tok.c_str());
}
/**
 * \brief Sends a IRC action (/me) to the user or channel
 * \param Destination Where the message will go
 * \param Message The message to send to Destination
 */
void IRCProto::action(const Flux::string &where, const char *fmt, ...)
{
  if(fmt)
  {
    va_list args;
    char buffer[BUFSIZE] = "";
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    this->action(where, Flux::string(buffer));
    va_end(args);
  }
}
/**
 * \overload void IRCProto::action(Flux::string where, Flux::string msg)
 * \brief Sends a IRC notice to the user or channel
 */
void IRCProto::action(const Flux::string &where, const Flux::string &msg)
{
 sepstream sep(msg, '\n');
 Flux::string tok;
 while(sep.GetToken(tok))
   this->Raw("PRIVMSG %s :\001ACTION %s\001\n", where.c_str(), tok.c_str());
}
/*****************************************************************************************/
/**
 * \fn void command::kick(Flux::string Channel, Flux::string User, const char *fmt, ...)
 * \brief Handles kick requests
 * \param channel Channel to be kicked from.
 * \param user User to be kicked.
 * \param reason Reason for the kick.
 */
void IRCProto::kick(const Flux::string &Channel, const Flux::string &User, const char *fmt, ...)
{
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  this->kick(Channel, User, Flux::string(buffer));
  va_end(args);
}
/**
 * \fn void command::topic(Flux::string channel, const char *fmt, ...)
 * \brief Sets channel topic.
 */
void IRCProto::topic(const Flux::string &channel, const char *fmt, ...)
{
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  this->topic(channel, Flux::string(buffer));
  va_end(args);
}
/**
 * \fn void IRCProto::quit(const char *fmt, ...)
 * \brief Handles quitting of irc
 * \param message Quit message
 */
void IRCProto::quit(const char *fmt, ...)
{
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  this->quit(Flux::string(buffer));
  va_end(args);
}

void IRCProto::ping(const char *fmt, ...)
{
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  this->ping(Flux::string(buffer));
  va_end(args);
}

/**
 * \fn void command::part(Flux::string channel, Flux::string reason)
 * \brief Sends part with message
 * \param channel Channel to part from.
 * \param reason Reason for parting.
 */
void IRCProto::part(const Flux::string &channel, const char *fmt, ...)
{
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  this->part(channel, Flux::string(buffer));
  va_end(args);
}
void IRCProto::nick(const char *fmt, ...)
{
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  this->nick(Flux::string(buffer));
  va_end(args);
}
/**
 * \overload void command::kick(Flux::string channel, Flux::string user, Flux::string reason)
 */
void IRCProto::kick(const Flux::string &chan, const Flux::string &userstr, const Flux::string &msg)
{
  this->Raw("KICK %s %s :%s\n", chan.c_str(), userstr.c_str(), msg.c_str());
}
/**
 * \overload void IRCProto::quit(Flux::string message)
 */
void IRCProto::quit(const Flux::string &message)
{
  if(this->sqt->NetworkReady())
  {
    if(!message.empty())
      this->net->s->Write("QUIT :%s\n", message.c_str());
    else
      this->net->s->Write("QUIT\n");
  }
}

void IRCProto::ping(const Flux::string &message)
{
  if(this->sqt->NetworkReady())
      this->net->s->Write("PING :%s\n", message.c_str());
}
/**
 * \overload void IRCProto::part(Flux::string channel, Flux::string msg)
 */
void IRCProto::part(const Flux::string &channel, const Flux::string &msg)
{
  this->Raw("PART %s :%s\n", channel.c_str(), msg.c_str());
}
/**
 * \overload void command::topic(Flux::string channel, Flux::string msg)
 */
void IRCProto::topic(const Flux::string &chan, const Flux::string &msg)
{
  this->Raw("TOPIC %s :%s\n", chan.c_str(), msg.c_str());
}
/**
 * \fn void command::nick(Flux::string nick)
 * \brief Sets the bots nickname in IRC.
 * \param nickname A Flux::string with the new nickname.
 */
void IRCProto::nick(const Flux::string &bnick){
  this->Raw("NICK %s\n", bnick.c_str());
}
void IRCProto::away(const Flux::string &message){
  this->Raw("AWAY :%s", message.c_str());
}
/**
 * \fn void command::oper(Flux::string oper, Flux::string password)
 * \brief Sends IRC command /oper
 */
void IRCProto::oper(const Flux::string &username, const Flux::string &password)
{
  this->Raw("OPER %s %s\n", username.c_str(), password.c_str());
}
/**
 * \fn void command::join(Flux::string chan)
 * \brief Makes the bot join a channel
 * \param stringy_chan A Flux::string with the channel you want to join.
 */
void IRCProto::join(const Flux::string &dchan)
{
  this->Raw("JOIN %s\n", dchan.c_str());
}
/**
 * \overload void command::part(Flux::string channel)
 * \brief Parts channel w/o reason.
 * \param channel Channel to part from.
 */
void IRCProto::part(const Flux::string &fchan)
{
  this->Raw("PART %s\n", fchan.c_str());
}
/**
 * \fn void IRCProto::who(Flux::string chan)
 * \brief Sends a /who to the channel
 * \param chan A Flux::string with the channel you want to /who.
 */
void IRCProto::who(const Flux::string &chan)
{
  this->Raw("WHO %s\n", chan.c_str());
}
/**
 * \fn void IRCProto::names(Flux::string &chan)
 * \brief Sends /names to the channel
 * \param chan A Flux::string with the channel you want to /names.
 */
void IRCProto::names(const Flux::string &chan)
{
  this->Raw("NAMES %s\n", chan.c_str());
}
/**
 * \fn void command::whois(Flux::string Nick)
 * \brief Sends a IRC Whois to Server.
 * \param Nick Nick to query
 */
void IRCProto::whois(const Flux::string &nickname)
{
  this->Raw("WHOIS %s\n", nickname.c_str());
}
/**
 * \fn void command::mode(Flux::string nickname, Flux::string mode, Flux::string user)
 * \brief Sends a mode to be set in IRC
 * \param nickname Nickname of who we are setting a more to.
 * \param mode The mode to set.
 */
void IRCProto::mode(const Flux::string &chan, const Flux::string &usermode, const Flux::string &usernick)
{
  this->Raw("MODE %s %s %s\n", chan.c_str(), usermode.c_str(), usernick.c_str());
}
/**
 * \overload void command::mode(Flux:;string dest, Flux::string mode)
 * \brief Sends a mode to the server
 * @param dest where to set the mode
 * @param mode mode to set
 */
void IRCProto::mode(const Flux::string &dest, const Flux::string &chanmode)
{
  this->Raw("MODE %s %s\n", dest.c_str(), chanmode.c_str());
}

/**
 * \fn void IRCProto::introduce_client(const Flux::string &nick, const Flux::string &ident, const Flux::string &realname)
 * \brief Sends the user gecos to the server on connect
 * \param nick The nickname to set on connect
 * \param ident The ident at the beginning of the IRC host.
 * \param realname The real name gecos used in irc.
 */
void IRCProto::introduce_client(const Flux::string &nickname, const Flux::string &ident, const Flux::string &realname)
{
  // Get our hostname of our system
  char hostname[256];
  gethostname(hostname, 256);

  this->Raw("NICK %s\n", nickname.c_str());
  this->Raw("USER %s %s %s :%s\n", ident.c_str(), hostname, net->hostname.c_str(), realname.c_str());

  // FIXME: this.
//   if(!Config->ServerPassword.empty())
//     this->Raw("PASS %s\n", Config->ServerPassword.c_str());
}

void IRCProto::invite(const Flux::string &nickname, const Flux::string &channel)
{
  this->Raw("INVITE %s %s\n", nickname.c_str(), channel.c_str());
}

void IRCProto::version(const Flux::string &server)
{
  if(server.empty())
    this->Raw("VERSION\n");
  else
    this->Raw("VERSION %s\n", server.c_str());
}

void IRCProto::stats(const Flux::string &query, const Flux::string &server)
{
  if(server.empty())
    this->Raw("STATS %s\n", query.c_str());
  else
    this->Raw("STATS %s %s\n", query.c_str(), server.c_str());
}

void IRCProto::links(const Flux::string &remote, const Flux::string &mask)
{
  if(remote.empty() && mask.empty())
    this->Raw("LINKS\n");
  else if (!remote.empty() && mask.empty())
    this->Raw("LINKS %s\n", remote.c_str());
  else if (!remote.empty() && !mask.empty())
    this->Raw("LINKS %s %s\n", remote.c_str(), mask.c_str());
}

void IRCProto::time(const Flux::string &server)
{
  if(server.empty())
    this->Raw("TIME\n");
  else
    this->Raw("TIME %s\n", server.c_str());
}

void IRCProto::admin(const Flux::string &server)
{
  if(server.empty())
    this->Raw("ADMIN\n");
  else
    this->Raw("ADMIN %s\n", server.c_str());
}

void IRCProto::info(const Flux::string &server)
{
  if(server.empty())
    this->Raw("INFO\n");
  else
    this->Raw("INFO %s\n", server.c_str());
}

void IRCProto::whowas(const Flux::string &nickname, int count, const Flux::string &server)
{
  if(count != 0 && !server.empty())
    this->Raw("WHOWAS %s %d %s\n", nickname.c_str(), count, server.c_str());
  else if (count != 0 && server.empty())
    this->Raw("WHOWAS %s %d\n", nickname.c_str());
  else if (count == 0 && !server.empty())
    this->Raw("WHOWAS %s %s\n", nickname.c_str(), server.c_str());
  else
    this->Raw("WHOWAS %s\n", nickname.c_str());
}

void IRCProto::users(const Flux::string &server)
{
  if(server.empty())
    this->Raw("USERS\n");
  else
    this->Raw("USERS %s\n", server.c_str());
}

void IRCProto::userhost(const Flux::string &nickname)
{
  this->Raw("USERHOST %s\n", nickname.c_str());
}

/*********************************************************************************/
/**************************** Global Protocol Class ******************************/
/*********************************************************************************/
GlobalProto::GlobalProto() {}

/**
 * \brief Sends a IRC private message to the user or channel
 * \param Destination Where the message will go
 * \param Message The message to send to Destination
 */
void GlobalProto::privmsg(const Flux::string &where, const char *fmt, ...)
{
  va_list args;
  char buffer[BUFSIZE] = "";
  if(fmt)
  {
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    this->privmsg(where, Flux::string(buffer));
    va_end(args);
  }
}
/**
 * \overload void GlobalProto::privmsg(Flux::string where, Flux::string msg)
 * \brief Sends a IRC private message to the user or channel
 */
void GlobalProto::privmsg(const Flux::string &where, const Flux::string &msg)
{
  sepstream sep(msg, '\n');
  Flux::string tok;
  while(sep.GetToken(tok))
    Send_Global("PRIVMSG %s :%s\n", where.c_str(), tok.c_str());
}
/**
 * \brief Sends a IRC notice to the user or channel
 * \param Destination Where the message will go
 * \param Message The message to send to Destination
 */
void GlobalProto::notice(const Flux::string &where, const char *fmt, ...)
{
  va_list args;
  char buffer[BUFSIZE] = "";
  if(fmt)
  {
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    this->notice(where, Flux::string(buffer));
    va_end(args);
  }
}
/**
 * \overload void GlobalProto::notice(Flux::string where, Flux::string msg)
 * \brief Sends a IRC notice to the user or channel
 */
void GlobalProto::notice(const Flux::string &where, const Flux::string &msg)
{
  sepstream sep(msg, '\n');
  Flux::string tok;
  while(sep.GetToken(tok))
    Send_Global("NOTICE %s :%s\n", where.c_str(), tok.c_str());
}
/**
 * \brief Sends a IRC action (/me) to the user or channel
 * \param Destination Where the message will go
 * \param Message The message to send to Destination
 */
void GlobalProto::action(const Flux::string &where, const char *fmt, ...)
{
  if(fmt)
  {
    va_list args;
    char buffer[BUFSIZE] = "";
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    this->action(where, Flux::string(buffer));
    va_end(args);
  }
}
/**
 * \overload void GlobalProto::action(Flux::string where, Flux::string msg)
 * \brief Sends a IRC notice to the user or channel
 */
void GlobalProto::action(const Flux::string &where, const Flux::string &msg)
{
  sepstream sep(msg, '\n');
  Flux::string tok;
  while(sep.GetToken(tok))
    Send_Global("PRIVMSG %s :\001ACTION %s\001\n", where.c_str(), tok.c_str());
}
/*****************************************************************************************/
/**
 * \fn void command::kick(Flux::string Channel, Flux::string User, const char *fmt, ...)
 * \brief Handles kick requests
 * \param channel Channel to be kicked from.
 * \param user User to be kicked.
 * \param reason Reason for the kick.
 */
void GlobalProto::kick(const Flux::string &Channel, const Flux::string &User, const char *fmt, ...)
{
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  this->kick(Channel, User, Flux::string(buffer));
  va_end(args);
}
/**
 * \fn void command::topic(Flux::string channel, const char *fmt, ...)
 * \brief Sets channel topic.
 */
void GlobalProto::topic(const Flux::string &channel, const char *fmt, ...)
{
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  this->topic(channel, Flux::string(buffer));
  va_end(args);
}
/**
 * \fn void GlobalProto::quit(const char *fmt, ...)
 * \brief Handles quitting of irc
 * \param message Quit message
 */
void GlobalProto::quit(const char *fmt, ...)
{
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  this->quit(Flux::string(buffer));
  va_end(args);
}
/**
 * \fn void command::part(Flux::string channel, Flux::string reason)
 * \brief Sends part with message
 * \param channel Channel to part from.
 * \param reason Reason for parting.
 */
void GlobalProto::part(const Flux::string &channel, const char *fmt, ...)
{
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  this->part(channel, Flux::string(buffer));
  va_end(args);
}
/**
 * \overload void command::kick(Flux::string channel, Flux::string user, Flux::string reason)
 */
void GlobalProto::kick(const Flux::string &chan, const Flux::string &userstr, const Flux::string &msg)
{
  Send_Global("KICK %s %s :%s\n", chan.c_str(), userstr.c_str(), msg.c_str());
}
/**
 * \overload void GlobalProto::quit(Flux::string message)
 */
void GlobalProto::quit(const Flux::string &message)
{
  Send_Global("QUIT :%s\n", message.c_str());
}
/**
 * \overload void GlobalProto::part(Flux::string channel, Flux::string msg)
 */
void GlobalProto::part(const Flux::string &channel, const Flux::string &msg)
{
  Send_Global("PART %s :%s\n", channel.c_str(), msg.c_str());
}
/**
 * \overload void command::topic(Flux::string channel, Flux::string msg)
 */
void GlobalProto::topic(const Flux::string &chan, const Flux::string &msg)
{
  Send_Global("TOPIC %s :%s\n", chan.c_str(), msg.c_str());
}
/**
 * \fn void command::nick(Flux::string nick)
 * \brief Sets the bots nickname in IRC.
 * \param nickname A Flux::string with the new nickname.
 */
void GlobalProto::nick(const Flux::string &bnick)
{
  Send_Global("NICK %s\n", bnick.c_str());
}
void GlobalProto::away(const Flux::string &message)
{
  Send_Global("AWAY :%s", message.c_str());
}
/**
 * \fn void command::oper(Flux::string oper, Flux::string password)
 * \brief Sends IRC command /oper
 */
void GlobalProto::oper(const Flux::string &username, const Flux::string &password)
{
  Send_Global("OPER %s %s\n", username.c_str(), password.c_str());
}
/**
 * \fn void command::join(Flux::string chan)
 * \brief Makes the bot join a channel
 * \param stringy_chan A Flux::string with the channel you want to join.
 */
void GlobalProto::join(const Flux::string &dchan)
{
  Send_Global("JOIN %s\n", dchan.c_str());
}
/**
 * \overload void command::part(Flux::string channel)
 * \brief Parts channel w/o reason.
 * \param channel Channel to part from.
 */
void GlobalProto::part(const Flux::string &fchan)
{
  Send_Global("PART %s\n", fchan.c_str());
}
/**
 * \fn void GlobalProto::who(Flux::string chan)
 * \brief Sends a /who to the channel
 * \param chan A Flux::string with the channel you want to /who.
 */
void GlobalProto::who(const Flux::string &chan)
{
  Send_Global("WHO %s\n", chan.c_str());
}
/**
 * \fn void GlobalProto::names(Flux::string &chan)
 * \brief Sends /names to the channel
 * \param chan A Flux::string with the channel you want to /names.
 */
void GlobalProto::names(const Flux::string &chan)
{
  Send_Global("NAMES %s\n", chan.c_str());
}
/**
 * \fn void command::whois(Flux::string Nick)
 * \brief Sends a IRC Whois to Server.
 * \param Nick Nick to query
 */
void GlobalProto::whois(const Flux::string &nickname)
{
  Send_Global("WHOIS %s\n", nickname.c_str());
}
/**
 * \fn void command::mode(Flux::string nickname, Flux::string mode, Flux::string user)
 * \brief Sends a mode to be set in IRC
 * \param nickname Nickname of who we are setting a more to.
 * \param mode The mode to set.
 */
void GlobalProto::mode(const Flux::string &chan, const Flux::string &usermode, const Flux::string &usernick)
{
  Send_Global("MODE %s %s %s\n", chan.c_str(), usermode.c_str(), usernick.c_str());
}
/**
 * \fn void GlobalProto::user(Flux::string ident, Flux::string realname)
 * \brief Sends the user gecos to the server
 * \param ident The ident at the beginning of the IRC host.
 * \param realname The real name gecos used in irc.
 */
void GlobalProto::user(const Flux::string &ident, const Flux::string &realname)
{
  Send_Global("USER %s * * :%s\n", ident.c_str(), realname.c_str());
}
/**
 * \overload void command::mode(Flux:;string dest, Flux::string mode)
 * \brief Sends a mode to the server
 * @param dest where to set the mode
 * @param mode mode to set
 */
void GlobalProto::mode(const Flux::string &dest, const Flux::string &chanmode)
{
  Send_Global("MODE %s %s\n", dest.c_str(), chanmode.c_str());
}

/*******************************************************************************/
/**
 * \overload void Send_Global(const Flux::string &str)
 */
void Send_Global(const Flux::string &str)
{
  Network *n;
  for(auto it : Networks){
    if((n = it.second) && n->s && n->s->GetStatus(SF_CONNECTED))
    {
	n->s->Write(str);
	n->s->ProcessWrite();
    }
    else
      Log(LOG_RAWIO) << '[' << (n?n->name:"Unknown") << ']' << " Attempted to send '" << str << "'";
  }
}
/**
 * \fn void Send_Global(const char *fmt, ...)
 * \brief Send globally to all networks
 * \param string The string to send to all networks
 * \param ... variable parameters for the string
 */
void Send_Global(const char *fmt, ...)
{
  if(fmt){
    char buffer[BUFSIZE] = "";
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    Send_Global(Flux::string(buffer));
    va_end(args);
  }
}