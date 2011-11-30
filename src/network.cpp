#include "network.h"

Network::Network(const Flux::string &host, const Flux::string &p, const Flux::string &n): name(n), hostname(host), port(p)
{
  Log(LOG_DEBUG) << "New network created: " << n << " " << host << ':' << p;
}

bool Network::Disconnect()
{
  Socket *tmp = this->s;
  IRCProto ptmp = this->ircproto;
  this->s = NULL;
  this->ircproto = NULL;
  delete ircproto;
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

/**********************************************************/
NetworkSocket::NetworkSocket(Network *tnet) : Socket(-1), ConnectionSocket(), BufferedSocket()
{
  this->net = tnet;
  tnet->s = this;
  Log(LOG_TERMINAL) << "New Network Socket for " << tnet->name << " connecting to " tnet->hostname << ':' << tnet->port;
}
NetworkSocket::~NetworkSocket()
{
  this->Write("QUIT :Socket Closed\n");
  this->ProcessWrite();
}
bool NetworkSocket::Read(const Flux::string &buf)
{
  Flux::vector params = StringVector(buf, ' ');
  if(!params.empty() && params[0].equals_ci("ERROR"))
  {
    FOREACH_MOD(I_OnSocketError, OnSocketError(buf));
    return false; //Socket is dead so we'll let the socket engine handle it
  }
  Log(LOG_TERMINAL) << "Socket " << this->GetFD() << ": " << buf;
  //process(buf); //Work on this later, for now we just log what we get
  if(!params.empty() && params[0].equals_ci("PING"))
    this->Write("PONG :"+params[1]);
  return true;
}

void NetworkSocket::OnConnect()
{
  Log(LOG_TERMINAL) << "Successfuly connected to " << this->net->name << " (" << this->net->hostname << ':' << this->net->port << ')';
  FOREACH_MOD(I_OnPostConnect, OnPostConnect(this, this->net));
  this->Write("derpy!");
  this->ProcessWrite();
}
void NetworkSocket::OnError(const Flux::string &buf)
{
  Log(LOG_TERMINAL) << "Unable to connect to " << this->net->name << " (" << this->net->hostname << ':' << this->net->port << ')' << (!buf.empty()?(": " + buf):"");
}
/**********************************************************/