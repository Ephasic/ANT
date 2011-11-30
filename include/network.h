#ifndef _NETWORK_H_
#define _NETWORK_H_
#include "module.h"
#include "bot.h"
#include "includes.h"

class Network;
struct CommitMessage
{
  Flux::vector MessageMeta;
  std::vector<Channel*> Channels;
  std::vector<Bot*> Bots;
  Network *network;
};

/* Base socket class for ALL network connections for the Network class */
class NetworkSocket : public ConnectionSocket, public BufferedSocket
{
  Network *net;
public:
  NetworkSocket(Network*);
  ~NetworkSocket();
  bool Read(const Flux::string&);
  void OnConnect();
  void OnError(const Flux::string&);
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
  bool Disconnect(const Flux::string&);
  bool Connect();
};

#endif