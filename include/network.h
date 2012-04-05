/* Arbitrary Navn Tool -- Network Class Prototype
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#pragma once
#ifndef NETWORK_H
#define NETWORK_H
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
  // The socket
  NetworkSocket *s;
  // a timer for when we are supposed to ping timeout
  PingTimeoutTimer *ptt;
  // When we join a network but aren't synced yet
  std::queue<Channel*> JoinQueue;
  // bot pointer for the network
  Bot *b;
  // Map of all users in the network
  Flux::insensitive_map<User*> UserNickList;
  // Map of all channels in the network
  Flux::insensitive_map<Channel*> ChanMap;
  // ircd information
  Flux::string ircdversion;
  // Network name
  Flux::string name;
  // Network hostname currently in use
  Flux::string hostname;
  // Usable hostnames
  std::map<int, Flux::string> hostnames;
  // Port to connect to
  Flux::string port;
  // name of the server we're connected to (ie. pulsar.azuru.net)
  Flux::string servername;
  int CurHost;
  void SetConnectedHostname(const Flux::string &str) { this->usedhostname = str; }
  Flux::string GetConHost() const { return this->usedhostname; }
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