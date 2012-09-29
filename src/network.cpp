/* Arbitrary Navn Tool -- Network class and NetworkSocket
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

#include "network.h"
#include "bot.h"
#include "INIReader.h"
#include "module.h"

Flux::insensitive_map<Network*> Networks;
Flux::map<Network*> NetworkHosts;

Network::Network(const Flux::string &host, const Flux::string &p, const Flux::string &n): disconnecting(false), issynced(false), RTimer(nullptr), s(nullptr), b(nullptr), hostname(host), port(p), CurHost(0)
{
  if(host.empty() || p.empty())
    throw CoreException("Network class created with incorrect parameters given");

  //If we didn't specify the network name, use the hostname.
  this->name = n.empty()?host:n;

  // TODO: Non-Blocking queries are a must
  DNSQuery rep = DNSManager::BlockingQuery(host, host.search(':') ? DNS_QUERY_AAAA : DNS_QUERY_A);
  this->hostnames[1] = !rep.answers.empty() ? rep.answers.front().rdata : host;
  Networks[this->name] = this;
  NetworkHosts[host] = this;

  Log(LOG_DEBUG) << "New network created: " << n << " " << host << ':' << p;
}

Network::~Network()
{
  Log(LOG_DEBUG) << "Deleting network " << this->name << " (" << this->hostname << ':' << this->port << ')';
  this->Disconnect("Network Removed");
  if(RTimer)
    delete RTimer;
  Networks.erase(this->name);
  NetworkHosts.erase(this->hostname);
}

bool Network::JoinChannel(const Flux::string &chan)
{
    Log(LOG_DEBUG) << "Scheduling Channel " << chan << " for join.";
    if(IsValidChannel(chan))
    {
	Channel *c = this->FindChannel(chan);
	if(!c)
	    c = new Channel(this, chan);
	if(!this->s || !this->s->GetStatus(SF_CONNECTED))
	    this->JoinQueue.push(c);
	else
	    c->SendJoin();
	return true;
    }
    return false;
}

bool Network::Disconnect()
{
  // Check if we have a socket to send on
  if(!this->s)
    return false;
  // Delete the bot object
  if(this->b)
    DeleteZero(this->b);
  // We'll let the socket engine delete the socket
  this->s->SetDead(true);
  // say this network is disconnecting
  this->disconnecting = true;
  // return that we did something
  return true;
}

bool Network::Disconnect(const char *fmt, ...)
{
  va_list args;
  char buffer[BUFSIZE] = "";
  if(fmt)
  {
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    this->Disconnect(Flux::string(buffer));
    va_end(args);
  }
  return true;
}

bool Network::Disconnect(const Flux::string &buf)
{
  if(!buf.empty() && this->s && this->b)
    this->b->ircproto->quit(buf);
  this->Disconnect();
  return true;
}

bool Network::IsSynced() const
{
  return this->issynced && this->s && this->s->GetStatus(SF_CONNECTED);
}

void Network::Sync()
{
    if(this->isupport.UserModes.search('B'))
	this->b->SetMode("+B"); //FIXME: get bot mode?
    if(this->isupport.IRCdVersion.search_ci("ircd-seven") && this->isupport.UserModes.search('Q'))
	this->b->SetMode("+Q"); //for freenode to stop those redirects

    sepstream cs(Config->Channel, ',');
    Flux::string tok;
    while(cs.GetToken(tok))
    {
	tok.trim();
	new Channel(this, tok);
    }

    // Join pending channels
    while(!this->JoinQueue.empty())
    {
	Channel *c = this->JoinQueue.front();
	this->JoinQueue.pop();
	Log(LOG_DEBUG) << "Joining " << c->name << " (" << this->name << ')';
	c->SendJoin();
    }

    this->servername = this->isupport.ServerHost;
    this->issynced = true;
    FOREACH_MOD(I_OnNetworkSync, OnNetworkSync(this));
    Log(LOG_DEBUG) << "Network " << this->name << " is synced!";
}

bool Network::Connect()
{
  this->disconnecting = false;
  // FIXME: ANT doesn't load the channels on a reconnect.
  EventResult e;
  FOREACH_RESULT(I_OnPreConnect, OnPreConnect(this), e);
  if(e != EVENT_CONTINUE)
    return false;

  if(!this->s)
    this->s = new NetworkSocket(this);
  return true;
}

User *Network::FindUser(const Flux::string &fnick)
{
  auto it = this->UserNickList.find(fnick);
  if(it != this->UserNickList.end())
    return it->second;
  return nullptr;
}

Channel *Network::FindChannel(const Flux::string &channel)
{
  auto it = this->ChanMap.find(channel);
  if(it != this->ChanMap.end())
    return it->second;
  return NULL;
}

Network *FindNetwork(const Flux::string &name)
{
  auto it = Networks.find(name);
  if(it != Networks.end())
    return it->second;
  return nullptr;
}

Network *FindNetworkByHost(const Flux::string &name)
{
  auto it = NetworkHosts.find(name);
  if(it != NetworkHosts.end())
    return it->second;
  return nullptr;
}
