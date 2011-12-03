#ifndef _NETWORK_H_
#define _NETWORK_H_
#include "module.h"
#include "bot.h"
#include "includes.h"

class NetworkSocket;
struct CommitMessage
{
  Flux::string project;
  Flux::string branch;
  Flux::string ScriptVersion
  Flux::string ScriptName;
  Flux::string ScriptURL;
  Flux::string module;
  Flux::string timestamp;
  Flux::string author;
  Flux::string revision;
  Flux::string log;
  Flux::string url;
  Flux::vector Files;
  std::vector<Channel*> Channels;
  std::vector<Bot*> Bots;
  Network *network;
};

class CoreExport Network
{
public:
  Network(const Flux::string&, const Flux::string&, const Flux::string &n = "");
  ~Network();
  NetworkSocket *s;
  IRCProto *ircproto;
  Flux::map<Bot*> bots;
  Flux::string name;
  Flux::string hostname;
  Flux::string port;
  Flux::string servername;
  bool Disconnect();
  bool Disconnect(const Flux::string&);
  bool Connect();
};
#endif