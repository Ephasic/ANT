#ifndef _NETWORK_H_
#define _NETWORK_H_
#include "module.h"
// #include "bot.h"
#include "timers.h"
// #include "includes.h"

class Bot;
class NetworkSocket;
struct CommitMessage
{
  Flux::string project;
  Flux::string branch;
  Flux::string ScriptVersion;
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
protected:
  bool disconnecting;
public:
  Network(const Flux::string&, const Flux::string&, const Flux::string &n = "");
  ~Network();
  NetworkSocket *s;
  IRCProto *ircproto;
  Flux::insensitive_map<Bot*> bots;
  Flux::insensitive_map<User*> UserNickList;
  Flux::insensitive_map<Channel*> ChanMap;
  Bot *findbot(const Flux::string&);
  Flux::string name;
  Flux::string hostname;
  Flux::string port;
  Flux::string servername;
  bool JoinChannel(const Flux::string&);
  bool IsDisconnecting() { return this->disconnecting; }
  bool Disconnect();
  bool Disconnect(const Flux::string&);
  bool Connect();
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