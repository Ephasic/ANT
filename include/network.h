/* Arbitrary Navn Tool -- Network Class Prototype
 * 
 * (C) 2011-2012 Flux-Net
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
  PingTimeoutTimer *ptt;
  Bot *b;
  Flux::insensitive_map<User*> UserNickList;
  Flux::insensitive_map<Channel*> ChanMap;
  Flux::string ircdversion;
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