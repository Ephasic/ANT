#ifndef _NETWORK_H_
#define _NETWORK_H_
#include "module.h"
#include "bots.h"

class Network;
struct CommitMessage
{
  std::vector<Flux::string> MessageMeta;
  std::vector<Channel*> Channels;
  std::vector<Bot*> Bots;
  Network *network;
}

class CoreExport Network
{
public:
  Network(const Flux::string &host, const Flux::string &p, const Flux::string &n = ""):hostname(host), port(p), name(n)
  {
    Log(LOG_DEBUG) << "New network created: " << n << " " << host << ':' << p;
  }
  Socket *s;
  Flux::map<Bot*> bots;
  Flux::string name;
  Flux::string hostname;
  Flux::string port;
  Flux::string servername;
  bool Disconnect();
  bool Connect();
};

#endif