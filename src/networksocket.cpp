/* Arbitrary Navn Tool -- NetworkSocket class
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
#include "INIReader.h"
#include "module.h"
#include "bot.h"

/**********************************************************/
/************************ Timers **************************/
/**********************************************************/

ReconnectTimer::ReconnectTimer(int wait, Network *net) : Timer(wait), n(net)
{
  if(!net)
    return; // Just ignore, we might be exiting from a CoreException

    n->RTimer = this;
}
void ReconnectTimer::Tick(time_t)
{
  try
  {
    if(!quitting)
      n->Connect();
  }
  catch (const SocketException &e)
  {
    n->s = nullptr; // XXX: Does this memleak?
    Log() << "Connection to " << n->name << " [" << n->GetConHost() << ':'
    << n->port << "] Failed! (" << e.GetReason() << ") Retrying in " << Config->RetryWait << " seconds.";

    new ReconnectTimer(Config->RetryWait, n);
    return;
  }
  n->RTimer = nullptr;
}

/**********************************************************/
/****************** Socket Engine *************************/
/**********************************************************/

NetworkSocket::NetworkSocket(Network *tnet) : Socket(-1), ConnectionSocket(), BufferedSocket(), net(tnet), SentPing(false)
{
  if(!tnet)
    throw CoreException("Network socket created with no network? lolwut?");

  this->net->CurHost++;
  if(static_cast<unsigned int>(this->net->CurHost) >= this->net->hostnames.size())
    this->net->CurHost = 1;
  
  this->net->SetConnectedHostname(this->net->hostnames[this->net->CurHost]);

  Log(LOG_TERMINAL) << "New Network Socket for " << tnet->name << " connecting to "
  << tnet->hostname << ':' << tnet->port << '(' << tnet->GetConHost() << ')';

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
    Log() << "Connection to " << net->name << " [" << net->GetConHost() << ':'
    << net->port << "] Failed! Retrying in " << Config->RetryWait << " seconds.";

    new ReconnectTimer(Config->RetryWait, this->net);
  }
}

bool NetworkSocket::Read(const Flux::string &buf)
{
  Log(LOG_RAWIO) << '[' << this->net->name << "] " << buf;
  Flux::vector params = ParamitizeString(buf, ' ');

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
  Log(LOG_TERMINAL) << "Successfully connected to " << this->net->name << " ["
  << this->net->hostname << ':' << this->net->port << "] (" << this->net->GetConHost() << ")";

  new Bot(this->net, printfify("%stmp%03d", Config->NicknamePrefix.strip('-').c_str(),
			       randint(0, 999)), Config->Ident, Config->Realname);

  this->net->b->introduce();
  FOREACH_MOD(I_OnPostConnect, OnPostConnect(this, this->net));
  this->ProcessWrite();
}

void NetworkSocket::OnError(const Flux::string &buf)
{
  Log(LOG_TERMINAL) << "Unable to connect to " << this->net->name << " ("
  << this->net->hostname << ':' << this->net->port << ')' << (!buf.empty()?(": " + buf):"");
}

bool NetworkSocket::ProcessWrite()
{
  Log(LOG_RAWIO) << '[' << this->net->name << ']' << ' ' << this->WriteBuffer;
  return ConnectionSocket::ProcessWrite() && BufferedSocket::ProcessWrite();
}