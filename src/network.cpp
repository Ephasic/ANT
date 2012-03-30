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

Flux::insensitive_map<Network*> Networks;
Flux::map<Network*> NetworkHosts;

Network::Network(const Flux::string &host, const Flux::string &p, const Flux::string &n): s(nullptr), b(nullptr), CurHost(0),
disconnecting(false), hostname(host), port(p)
{
  if(host.empty() || p.empty())
    throw CoreException("Network class created with incorrect parameters given");

  //If we didn't specify the network name, use the hostname.
  this->name = n.empty()?host:n;
  this->hostnames = ForwardResolution(host);
  Networks[this->name] = this;
  NetworkHosts[host] = this;
  
  Log(LOG_DEBUG) << "New network created: " << n << " " << host << ':' << p;
}

Network::~Network()
{
  Log(LOG_DEBUG) << "Deleting network " << this->name << " (" << this->hostname << ':' << this->port << ')';
  this->Disconnect("Network Removed");
  Networks.erase(this->name);
  NetworkHosts.erase(this->hostname);
}

bool Network::JoinChannel(const Flux::string &chan)
{
  Log(LOG_DEBUG) << "Scheduling Channel " << chan << " for join.";
  if(IsValidChannel(chan))
  {
    Channel *c = FindChannel(this, chan);
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
  Socket *tmp = this->s;
  Bot *bot = this->b;
  this->disconnecting = true;
  this->s = nullptr;
  this->b = nullptr;
  delete bot;
  delete tmp;
  return true;
}

bool Network::Disconnect(const Flux::string &buf)
{
  if(!buf.empty() && this->s)
    this->b->ircproto->quit(buf);
  this->Disconnect();
  return true;
}

bool Network::Connect()
{
  this->disconnecting = false;
  // FIXME: ANT doesn't load the channels on a reconnect.
  FOREACH_MOD(I_OnPreConnect, OnPreConnect(this));
  if(!this->s)
    this->s = new NetworkSocket(this);
  return true;
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

/**********************************************************/
/************************ Timers **************************/
/**********************************************************/

PingTimeoutTimer::PingTimeoutTimer(Network *net) : Timer(121, time(NULL)), n(net) { net->ptt = this; }
void PingTimeoutTimer::Tick(time_t)
{
  Log(LOG_RAWIO) << n->name << ": Ping Timeout";
  if(n->s && n->s->GetStatus(SF_CONNECTED) && n->s->SentPing)
    n->s->SetDead(true);
}

ReconnectTimer::ReconnectTimer(int wait, Network *net) : Timer(wait), n(net) {}
void ReconnectTimer::Tick(time_t)
{
  try
  {
    n->Connect();
  }
  catch (const SocketException &e)
  {
    n->s = nullptr; // XXX: Does this memleak?
    Log() << "Connection to " << n->name << " [" << n->GetConHost() << ':' << n->port << "] Failed! (" << e.GetReason() << ") Retrying in " << Config->RetryWait << " seconds.";
    new ReconnectTimer(Config->RetryWait, n);
  }
}

/**********************************************************/
/****************** Socket Engine *************************/
/**********************************************************/

NetworkSocket::NetworkSocket(Network *tnet) : Socket(-1), ConnectionSocket(), BufferedSocket(), net(tnet), SentPing(false)
{
  if(!tnet)
    throw CoreException("Network socket created with no network? lolwut?");
  
  this->net->SetConnectedHostname(this->net->hostnames[++this->net->CurHost]);
  
  Log(LOG_TERMINAL) << "New Network Socket for " << tnet->name << " connecting to " << tnet->hostname << ':' << tnet->port << '(' << tnet->GetConHost() << ')';
  
  this->Connect(tnet->GetConHost(), tnet->port);
}

NetworkSocket::~NetworkSocket()
{
  this->Write("QUIT :Socket Closed\n");
  this->ProcessWrite();
  this->net->s = nullptr;
  
  Log() << "Closing Connection to " << net->name;
  
  if(!this->net->IsDisconnecting())
  {
    Log() << "Connection to " << net->name << " [" << net->GetConHost() << ':' << net->port << "] Failed! Retrying in " << Config->RetryWait << " seconds.";
    new ReconnectTimer(Config->RetryWait, this->net);
  }
}

bool NetworkSocket::Read(const Flux::string &buf)
{
  Log(LOG_RAWIO) << '[' << this->net->name << ']' << ' ' << buf;
  Flux::vector params = StringVector(buf, ' ');
  
  if(!params.empty() && params[0].search_ci("ERROR"))
  {
    FOREACH_MOD(I_OnSocketError, OnSocketError(buf));
    Log(LOG_TERMINAL) << "Socket Error, Closing socket!";
    return false; //Socket is dead so we'll let the socket engine handle it
  }
  
  process(this->net, buf);
  
  if(!params.empty() && params[0].equals_ci("PING"))
  {
    this->Write("PONG :"+params[1]);
    this->ProcessWrite();
  }
  return true;
}

void NetworkSocket::OnConnect()
{
  Log(LOG_TERMINAL) << "Successfuly connected to " << this->net->name << " [" << this->net->hostname << ':' << this->net->port << "] (" << this->net->GetConHost() << ")";
  
  new Bot(this->net, printfify("%stmp%03d", Config->NicknamePrefix.strip('-').c_str(), randint(0, 999)), Config->Ident, Config->Realname);
  
  this->net->b->SendUser();
  FOREACH_MOD(I_OnPostConnect, OnPostConnect(this, this->net));
  this->ProcessWrite();
}

void NetworkSocket::OnError(const Flux::string &buf)
{
  Log(LOG_TERMINAL) << "Unable to connect to " << this->net->name << " (" << this->net->hostname << ':' << this->net->port << ')' << (!buf.empty()?(": " + buf):"");
}

bool NetworkSocket::ProcessWrite()
{
  Log(LOG_RAWIO) << '[' << this->net->name << ']' << ' ' << this->WriteBuffer;
  return ConnectionSocket::ProcessWrite() && BufferedSocket::ProcessWrite();
}
/**********************************************************/