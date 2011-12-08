#include "network.h"

Flux::insensitive_map<Network*> Networks;
Flux::map<Network*> NetworkHosts;
Network::Network(const Flux::string &host, const Flux::string &p, const Flux::string &n)
{
  if(host.empty() || p.empty() || n.empty())
    throw CoreException("Network class created with incorrect parameters given");
  
  this->hostname = host;
  this->name = n;
  this->port = p;
  Networks[n] = this;
  NetworkHosts[host] = this;
  Log(LOG_DEBUG) << "New network created: " << n << " " << host << ':' << p;
}

Network::~Network()
{
  Log(LOG_DEBUG) << "Deleting network " << this->name << " (" << this->hostname << ':' << this->port << ')';
  Networks.erase(this->name);
  NetworkHosts.erase(this->hostname);
}

bool Network::Disconnect()
{
  Socket *tmp = this->s;
  IRCProto *ptmp = this->ircproto;
  this->s = NULL;
  this->ircproto = NULL;
  delete ptmp;
  delete tmp;
  return true;
}

bool Network::Disconnect(const Flux::string &buf)
{
  if(!buf.empty() && this->s)
    this->ircproto->quit(buf);
  this->Disconnect();
  return true;
}

bool Network::Connect()
{
  FOREACH_MOD(I_OnPreConnect, OnPreConnect(this));
  new NetworkSocket(this);
  return true;
}

Network *FindNetwork(const Flux::string &name)
{
  auto it = Networks.find(name);
  if(it != Networks.end())
    return it->second;
  return NULL;
}

Network *FindNetworkByHost(const Flux::string &name)
{
  auto it = NetworkHosts.find(name);
  if(it != NetworkHosts.end())
    return it->second;
  return NULL;
}
/**********************************************************/

class ReconnectTimer : public Timer
{
  Network *n;
public:
  ReconnectTimer(int wait, Network *net) : Timer(wait), n(net) { }
  void Tick(time_t)
  {
    try
    {
      n->Connect();
    }
    catch (const SocketException &e)
    {
      Log() << "Connection to " << n->name << " [" << n->hostname << ':' << n->port << "] Failed! (" << e.GetReason() << ") Retrying in " << this->GetSecs() << " seconds.";
    }
  }
};

NetworkSocket::NetworkSocket(Network *tnet) : Socket(-1), ConnectionSocket(), BufferedSocket(), net(tnet)
{
  tnet->s = this;
  Log(LOG_TERMINAL) << "New Network Socket for " << tnet->name << " connecting to " << tnet->hostname << ':' << tnet->port;
}

NetworkSocket::~NetworkSocket()
{
  this->Write("QUIT :Socket Closed\n");
  this->ProcessWrite();
  this->net->s = NULL;
  Log() << "Connection to " << net->name << " [" << net->hostname << ':' << net->port << "] Failed! Retrying in 30 seconds.";
  new ReconnectTimer(30, this->net);
}

bool NetworkSocket::Read(const Flux::string &buf)
{
  Log(LOG_RAWIO) << '[' << this->net->name << ']' << ' ' << buf;
  Flux::vector params = StringVector(buf, ' ');
  if(buf.search_ci("ERROR"))
  {
    FOREACH_MOD(I_OnSocketError, OnSocketError(buf));
    Log(LOG_TERMINAL) << "Socket Error, Closing socket!";
    return true; //Socket is dead so we'll let the socket engine handle it
  }
//   process(this->net, buf);
  if(!params.empty() && params[0].equals_ci("PING"))
    this->Write("PONG :"+params[1]);
  return true;
}

void NetworkSocket::OnConnect()
{
  Log(LOG_TERMINAL) << "Successfuly connected to " << this->net->name << " (" << this->net->hostname << ':' << this->net->port << ')';
  FOREACH_MOD(I_OnPostConnect, OnPostConnect(this, this->net));
  this->Write("USER %s * * :%s", Config->Ident.c_str(), Config->Realname.c_str());
  this->Write("NICK ANT-%i", randint(1,100));
  new IRCProto(this->net); // Create the new protocol class for the network
}

void NetworkSocket::OnError(const Flux::string &buf)
{
  Log(LOG_TERMINAL) << "Unable to connect to " << this->net->name << " (" << this->net->hostname << ':' << this->net->port << ')' << (!buf.empty()?(": " + buf):"");
}

bool NetworkSocket::ProcessWrite()
{
  Log(LOG_RAWIO) << '[' << this->net->name << ']' << ' ' << this->WriteBuffer;
  return true;
}
/**********************************************************/