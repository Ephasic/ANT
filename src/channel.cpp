/* Arbitrary Navn Tool -- Channel Handler and functions
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#include "channel.h"
#include "bot.h"
// name, topic, topic_setter, n, topic_time, creation_time
Channel::Channel(Network *net, const Flux::string &nname, time_t ts): name(nname), n(net), topic_time(0), creation_time(ts)
{
  if(nname.empty())
    throw CoreException("I don't like empty channel names in my channel constructor >:d");
  if(!IsValidChannel(nname))
    throw CoreException("An Invalid channel was passed into the Channel constructor :<");
  if(!net)
    throw CoreException("Channel \""+nname+"\" created with no network!");

  this->n->ChanMap[this->name] = this;
  Log(LOG_DEBUG) << "Created new channel '" << nname << "' on " << net->name;
}

Channel::~Channel()
{
  for(auto it : this->UserList)
    it.first->DelChan(this);

  this->UserList.clear();
  Log(LOG_DEBUG) << "Deleted channel " << this->name;
  this->n->ChanMap.erase(this->name);
}

User *Channel::finduser(Network *net, const Flux::string &usr)
{
  auto it1 = net->UserNickList.find(usr);
  User *u = it1->second;
  
  if(!u)
    return nullptr;
  
  UList::iterator it = UserList.find(u);
  
  if(it != UserList.end())
    return it->first;
  
  return nullptr;
}

void Channel::SendJoin()
{
  if(!this->n || !this->n->b)
    return; // incase we have some weird shit
  
  this->n->b->ircproto->join(this->name);
  this->SendWho();
}
void Channel::AddUser(User *u) { if(u) this->UserList[u] = this; }
void Channel::DelUser(User *u)
{
  UList::iterator it = UserList.find(u);
  if(it != UserList.end())
    UserList.erase(it);
}

void Channel::SendPart(const Flux::string &reason) { this->n->b->ircproto->part(this->name, reason); }
void Channel::SendPart(const char *fmt, ...)
{
  if(fmt)
  {
    char buffer[BUFSIZE] = "";
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    this->SendPart(Flux::string(buffer));
    va_end(args);
  }
}

void Channel::SendMessage(const Flux::string &message){ this->n->b->ircproto->privmsg(this->name, message); }
void Channel::SendMessage(const char *fmt, ...)
{
  if(fmt)
  {
    char buffer[BUFSIZE] = "";
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    this->SendMessage(Flux::string(buffer));
    va_end(args);
  }
}

void Channel::SendAction(const Flux::string &message) { this->n->b->ircproto->action(this->name, message); }
void Channel::SendAction(const char *fmt, ...)
{
  if(fmt)
  {
    char buffer[BUFSIZE] = "";
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    this->SendAction(Flux::string(buffer));
    va_end(args);
  }
}

void Channel::SendNotice(const Flux::string &message){ this->n->b->ircproto->notice(this->name, message); }
void Channel::SendNotice(const char *fmt, ...)
{
  if(fmt)
  {
    char buffer[BUFSIZE] = "";
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    this->SendNotice(Flux::string(buffer));
    va_end(args);
  }
}

void Channel::SendWho()
{
  if(!this->n || !this->n->b)
    return; // incase we have some weird shit
    
  this->n->b->ircproto->who(this->name);
}
/****************************************************************/
void QuitUser(Network *n, User *u)
{
  if(!u)
    return;
  for(auto var : n->ChanMap)
    for(auto var1 : var.second->UserList)
      if(var1.first == u)
	var1.second->DelUser(u);
    delete u;
}

void JoinChansInBuffer(Network *n)
{
  while(!n->JoinQueue.empty())
  {
    Channel *c = n->JoinQueue.front();
    n->JoinQueue.pop();
    Log(LOG_DEBUG) << "Joining " << c->name << " (" << c->n->name << ')';
    c->SendJoin();
  }
}

Channel *FindChannel(Network *n, const Flux::string &channel)
{
  auto it = n->ChanMap.find(channel);
  if(it != n->ChanMap.end())
    return it->second;
  return NULL;
}