/* Arbitrary Navn Tool -- Network Class Prototype
 * 
 * (C) 2011-2012 Azuru
 * Contact us at Dev@Flux-Net.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#ifndef _NETWORK_H_
#define _NETWORK_H_
#include "module.h"
#include "timers.h"

class Bot;
class NetworkSocket;

struct CommitMessage
{
  Flux::map<Flux::string> info;
  Flux::vector Files;
  std::vector<Channel*> Channels;
  std::vector<Bot*> Bots;
  Network *network;
};

class CoreExport Network : public Base
{
protected:
  bool disconnecting;
  Flux::string usedhostname;
public:
  Network(const Flux::string&, const Flux::string&, const Flux::string &n = "");
  ~Network();
  NetworkSocket *s;
  PingTimeoutTimer *ptt;
  std::queue<Channel*> JoinQueue;
  Bot *b;
  Flux::insensitive_map<User*> UserNickList;
  Flux::insensitive_map<Channel*> ChanMap;
  Flux::string ircdversion;
  Flux::string name;
  Flux::string hostname;
  std::map<int, Flux::string> hostnames;
  Flux::string port;
  Flux::string servername;
  int CurHost;
  void SetConnectedHostname(const Flux::string &str) { this->usedhostname = str; }
  Flux::string GetConHost() { return this->usedhostname; }
  bool JoinChannel(const Flux::string&);
  bool IsDisconnecting() { return this->disconnecting; }
  bool Disconnect();
  bool Disconnect(const Flux::string&);
  bool Connect();
};

class PingTimeoutTimer : public Timer
{
  Network *n;
public:
  PingTimeoutTimer(Network *net);
  void Tick(time_t);
};

class ReconnectTimer : public Timer
{
  Network *n;
public:
  ReconnectTimer(int, Network*);
  void Tick(time_t);
};
// NetworkSocket is in socket.h :/
#endif