#ifndef _NETWORK_H_
#define _NETWORK_H_
#include "module.h"
#include "bot.h"

class Network;
struct CommitMessage
{
  std::vector<Flux::string> MessageMeta;
  std::vector<Channel*> Channels;
  std::vector<Bot*> Bots;
  Network *network;
};

class CoreExport Network
{
public:
  Network(const Flux::string&, const Flux::string&, const Flux::string &n = "");
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