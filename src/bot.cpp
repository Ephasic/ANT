/* Arbitrary Navn Tool -- Bot Functions and Commit Parsing.
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

#include "bot.h"
#include "INIReader.h"

Bot::Bot(Network *net, const Flux::string &ni, const Flux::string &i, const Flux::string &real): User(net, ni, i, net->hostname, real), network(net), BotNum(1)
{
  if(!net)
    throw CoreException("Bot with no network??");
  if(ni.empty())
    throw CoreException("Bot with no nickname??");
  if(i.empty())
    throw CoreException("Bot with no ident??");
  if(net->b)
  {
    Log(LOG_WARN) << "Bot assigned to a network with a bot already assigned??";
    return; //close the constructor instead of throwing.
//     throw CoreException("Bot assigned to a network with a bot already assigned??");
  }

  this->n->b = this;
  this->ircproto = new IRCProto(this->n);
  Log(LOG_DEBUG) << "New bot created on " << net->name << ": " << ni << i << ": " << real;
}

Bot::~Bot()
{
  this->n->b = nullptr;
  if(this->ircproto)
    delete this->ircproto;
  Log(LOG_DEBUG) << "Bot " << this->nick << " for network " << this->n->name << " deleted!";
}

void Bot::Join(Channel *c)
{
  if(c)
    c->SendJoin();
}

void Bot::Join(const Flux::string &chan)
{
  Channel *c = this->n->FindChannel(chan);
  if(!c)
    c = new Channel(this->n, chan);
  c->SendJoin();
}

void Bot::SetMode(const Flux::string &modestr)
{
  if(this->ircproto)
    this->ircproto->mode(this->nick, modestr);
}


void Bot::Part(Channel *c, const Flux::string &message)
{
  if(!c->finduser(this->n, this->nick))
    return;

  c->SendPart(message);
}

void Bot::Quit(const Flux::string &message)
{
  if(this->ircproto)
    this->ircproto->quit(message);
}

void Bot::SetNick(const Flux::string &nickname)
{
  this->ircproto->nick(nickname);
}

void Bot::introduce()
{
  this->ircproto->introduce_client(this->nick, this->ident, this->realname);
}

// Make sure our nickname is still valid.
void Bot::CheckNickname()
{
  if(this->nick.search_ci("tmp"))
    this->SetNick(Config->NicknamePrefix + value_cast<Flux::string>(BotNum));

  if(!this->nick.equals_cs(Config->NicknamePrefix + value_cast<Flux::string>(BotNum)))
    this->SetNick(Config->NicknamePrefix + value_cast<Flux::string>(BotNum));
}

Bot *FindBot(const Flux::string &nick)
{
  for(auto it : Networks)
  {
    if(it.second->b != nullptr && it.second->b->nick.equals_ci(nick))
      return it.second->b;
  }
  return nullptr;
}

bool IsBot(User *u)
{
  if(!u)
    return false;

  for(auto it : Networks)
  {
    if(it.second->b != nullptr && it.second->b == dynamic_cast<Bot*>(u))
      return true;
  }

  return false;
}