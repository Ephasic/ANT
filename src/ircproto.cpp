/* All code is licensed under GNU General Public License GPL v3 (http://www.gnu.org/licenses/gpl.html) */
#include "ircproto.h"
/**
 *\file  ircproto.cpp
 *\brief Contains the IRCProto class.
 */
IRCProto::IRCProto(Network *n) : net(n) { n->ircproto = this; } //Because we have multiple networks, we need this to be dynamic with the sockets
/**
 * \brief Sends a IRC private message to the user or channel
 * \param Destination Where the message will go
 * \param Message The message to send to Destination 
 */
void IRCProto::privmsg(const Flux::string &where, const char *fmt, ...){
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
void IRCProto::privmsg(const Flux::string &where, const Flux::string &msg){
 sepstream sep(msg, '\n');
 Flux::string tok;
 while(sep.GetToken(tok))
   this->net->s->Write("PRIVMSG %s :%s\n", where.c_str(), tok.c_str());
}
/**
 * \brief Sends a IRC notice to the user or channel
 * \param Destination Where the message will go
 * \param Message The message to send to Destination 
 */
void IRCProto::notice(const Flux::string &where, const char *fmt, ...){
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
void IRCProto::notice(const Flux::string &where, const Flux::string &msg){
 sepstream sep(msg, '\n');
 Flux::string tok;
 while(sep.GetToken(tok))
   this->net->s->Write("NOTICE %s :%s\n", where.c_str(), tok.c_str());
}
/**
 * \brief Sends a IRC action (/me) to the user or channel
 * \param Destination Where the message will go
 * \param Message The message to send to Destination 
 */
void IRCProto::action(const Flux::string &where, const char *fmt, ...){
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
void IRCProto::action(const Flux::string &where, const Flux::string &msg){
 sepstream sep(msg, '\n');
 Flux::string tok;
 while(sep.GetToken(tok))
   this->net->s->Write("PRIVMSG %s :\001ACTION %s\001\n", where.c_str(), tok.c_str());
}
/*****************************************************************************************/
/**
 * \fn void command::kick(Flux::string Channel, Flux::string User, const char *fmt, ...)
 * \brief Handles kick requests
 * \param channel Channel to be kicked from.
 * \param user User to be kicked.
 * \param reason Reason for the kick.
 */
void IRCProto::kick(const Flux::string &Channel, const Flux::string &User, const char *fmt, ...){
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
void IRCProto::topic(const Flux::string &channel, const char *fmt, ...){
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
void IRCProto::quit(const char *fmt, ...){
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
void IRCProto::part(const Flux::string &channel, const char *fmt, ...){
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
void IRCProto::kick(const Flux::string &chan, const Flux::string &userstr, const Flux::string &msg){
  this->net->s->Write("KICK %s %s :%s\n", chan.c_str(), userstr.c_str(), msg.c_str());
}
/**
 * \overload void IRCProto::quit(Flux::string message)
 */
void IRCProto::quit(const Flux::string &message){
  this->net->s->Write("QUIT :%s\n", message.c_str());
}
/**
 * \overload void IRCProto::part(Flux::string channel, Flux::string msg)
 */
void IRCProto::part(const Flux::string &channel, const Flux::string &msg){
  this->net->s->Write("PART %s :%s\n", channel.c_str(), msg.c_str());
}
/**
 * \overload void command::topic(Flux::string channel, Flux::string msg)
 */
void IRCProto::topic(const Flux::string &chan, const Flux::string &msg){
  this->net->s->Write("TOPIC %s :%s\n", chan.c_str(), msg.c_str());
}
/**
 * \fn void command::nick(Flux::string nick)
 * \brief Sets the bots nickname in IRC.
 * \param nickname A Flux::string with the new nickname.
 */
void IRCProto::nick(const Flux::string &bnick){
  this->net->s->Write("NICK %s\n", bnick.c_str());
}
void IRCProto::away(const Flux::string &message){
  this->net->s->Write("AWAY :%s", message.c_str());
}
/**
 * \fn void command::oper(Flux::string oper, Flux::string password)
 * \brief Sends IRC command /oper
 */
void IRCProto::oper(const Flux::string &username, const Flux::string &password){
  this->net->s->Write("OPER %s %s\n", username.c_str(), password.c_str());
}
/**
 * \fn void command::join(Flux::string chan)
 * \brief Makes the bot join a channel
 * \param stringy_chan A Flux::string with the channel you want to join.
 */
void IRCProto::join(const Flux::string &dchan){
  this->net->s->Write("JOIN %s\n", dchan.c_str());
}
/**
 * \overload void command::part(Flux::string channel)
 * \brief Parts channel w/o reason.
 * \param channel Channel to part from.
 */
void IRCProto::part(const Flux::string &fchan){
  this->net->s->Write("PART %s\n", fchan.c_str());
}
/**
 * \fn void IRCProto::who(Flux::string chan)
 * \brief Sends a /who to the channel
 * \param chan A Flux::string with the channel you want to /who.
 */
void IRCProto::who(const Flux::string &chan){
  this->net->s->Write("WHO %s\n", chan.c_str());
}
/**
 * \fn void IRCProto::names(Flux::string &chan)
 * \brief Sends /names to the channel
 * \param chan A Flux::string with the channel you want to /names.
 */
void IRCProto::names(const Flux::string &chan){
  this->net->s->Write("NAMES %s\n", chan.c_str());
}
/**
 * \fn void command::whois(Flux::string Nick)
 * \brief Sends a IRC Whois to Server.
 * \param Nick Nick to query
 */
void IRCProto::whois(const Flux::string &nickname){
  this->net->s->Write("WHOIS %s\n", nickname.c_str());
}
/**
 * \fn void command::mode(Flux::string nickname, Flux::string mode, Flux::string user)
 * \brief Sends a mode to be set in IRC
 * \param nickname Nickname of who we are setting a more to.
 * \param mode The mode to set.
 */
void IRCProto::mode(const Flux::string &chan, const Flux::string &usermode, const Flux::string &usernick){
  this->net->s->Write("MODE %s %s %s\n", chan.c_str(), usermode.c_str(), usernick.c_str());
}
/**
 * \fn void IRCProto::user(Flux::string ident, Flux::string realname)
 * \brief Sends the user gecos to the server
 * \param ident The ident at the beginning of the IRC host.
 * \param realname The real name gecos used in irc.
 */
void IRCProto::user(const Flux::string &ident, const Flux::string &realname){
  this->net->s->Write("USER %s * * :%s\n", ident.c_str(), realname.c_str());
}
/**
 * \overload void command::mode(Flux:;string dest, Flux::string mode)
 * \brief Sends a mode to the server
 * @param dest where to set the mode
 * @param mode mode to set
 */
void IRCProto::mode(const Flux::string &dest, const Flux::string &chanmode){
  this->net->s->Write("MODE %s %s\n", dest.c_str(), chanmode.c_str());
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
void GlobalProto::privmsg(const Flux::string &where, const char *fmt, ...){
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
void GlobalProto::privmsg(const Flux::string &where, const Flux::string &msg){
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
void GlobalProto::notice(const Flux::string &where, const char *fmt, ...){
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
void GlobalProto::notice(const Flux::string &where, const Flux::string &msg){
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
void GlobalProto::action(const Flux::string &where, const char *fmt, ...){
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
void GlobalProto::action(const Flux::string &where, const Flux::string &msg){
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
void GlobalProto::kick(const Flux::string &Channel, const Flux::string &User, const char *fmt, ...){
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
void GlobalProto::topic(const Flux::string &channel, const char *fmt, ...){
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
void GlobalProto::quit(const char *fmt, ...){
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
void GlobalProto::part(const Flux::string &channel, const char *fmt, ...){
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
void GlobalProto::kick(const Flux::string &chan, const Flux::string &userstr, const Flux::string &msg){
  Send_Global("KICK %s %s :%s\n", chan.c_str(), userstr.c_str(), msg.c_str());
}
/**
 * \overload void GlobalProto::quit(Flux::string message)
 */
void GlobalProto::quit(const Flux::string &message){
  Send_Global("QUIT :%s\n", message.c_str());
}
/**
 * \overload void GlobalProto::part(Flux::string channel, Flux::string msg)
 */
void GlobalProto::part(const Flux::string &channel, const Flux::string &msg){
  Send_Global("PART %s :%s\n", channel.c_str(), msg.c_str());
}
/**
 * \overload void command::topic(Flux::string channel, Flux::string msg)
 */
void GlobalProto::topic(const Flux::string &chan, const Flux::string &msg){
  Send_Global("TOPIC %s :%s\n", chan.c_str(), msg.c_str());
}
/**
 * \fn void command::nick(Flux::string nick)
 * \brief Sets the bots nickname in IRC.
 * \param nickname A Flux::string with the new nickname.
 */
void GlobalProto::nick(const Flux::string &bnick){
  Send_Global("NICK %s\n", bnick.c_str());
}
void GlobalProto::away(const Flux::string &message){
  Send_Global("AWAY :%s", message.c_str());
}
/**
 * \fn void command::oper(Flux::string oper, Flux::string password)
 * \brief Sends IRC command /oper
 */
void GlobalProto::oper(const Flux::string &username, const Flux::string &password){
  Send_Global("OPER %s %s\n", username.c_str(), password.c_str());
}
/**
 * \fn void command::join(Flux::string chan)
 * \brief Makes the bot join a channel
 * \param stringy_chan A Flux::string with the channel you want to join.
 */
void GlobalProto::join(const Flux::string &dchan){
  Send_Global("JOIN %s\n", dchan.c_str());
}
/**
 * \overload void command::part(Flux::string channel)
 * \brief Parts channel w/o reason.
 * \param channel Channel to part from.
 */
void GlobalProto::part(const Flux::string &fchan){
  Send_Global("PART %s\n", fchan.c_str());
}
/**
 * \fn void GlobalProto::who(Flux::string chan)
 * \brief Sends a /who to the channel
 * \param chan A Flux::string with the channel you want to /who.
 */
void GlobalProto::who(const Flux::string &chan){
  Send_Global("WHO %s\n", chan.c_str());
}
/**
 * \fn void GlobalProto::names(Flux::string &chan)
 * \brief Sends /names to the channel
 * \param chan A Flux::string with the channel you want to /names.
 */
void GlobalProto::names(const Flux::string &chan){
  Send_Global("NAMES %s\n", chan.c_str());
}
/**
 * \fn void command::whois(Flux::string Nick)
 * \brief Sends a IRC Whois to Server.
 * \param Nick Nick to query
 */
void GlobalProto::whois(const Flux::string &nickname){
  Send_Global("WHOIS %s\n", nickname.c_str());
}
/**
 * \fn void command::mode(Flux::string nickname, Flux::string mode, Flux::string user)
 * \brief Sends a mode to be set in IRC
 * \param nickname Nickname of who we are setting a more to.
 * \param mode The mode to set.
 */
void GlobalProto::mode(const Flux::string &chan, const Flux::string &usermode, const Flux::string &usernick){
  Send_Global("MODE %s %s %s\n", chan.c_str(), usermode.c_str(), usernick.c_str());
}
/**
 * \fn void GlobalProto::user(Flux::string ident, Flux::string realname)
 * \brief Sends the user gecos to the server
 * \param ident The ident at the beginning of the IRC host.
 * \param realname The real name gecos used in irc.
 */
void GlobalProto::user(const Flux::string &ident, const Flux::string &realname){
  Send_Global("USER %s * * :%s\n", ident.c_str(), realname.c_str());
}
/**
 * \overload void command::mode(Flux:;string dest, Flux::string mode)
 * \brief Sends a mode to the server
 * @param dest where to set the mode
 * @param mode mode to set
 */
void GlobalProto::mode(const Flux::string &dest, const Flux::string &chanmode){
  Send_Global("MODE %s %s\n", dest.c_str(), chanmode.c_str());
}

/*******************************************************************************/

/**
 * \overload void Send_Global(const Flux::string &str)
 */
void Send_Global(const Flux::string &str)
{
  for(auto it : Networks)
    if(it.second->s)
      it.second->s->Write(str);
    else
      Log(LOG_RAWIO) << '[' << it.second->name << ']' << " Attempted to send '" << str << "'";
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