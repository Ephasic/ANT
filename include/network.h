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
#include "Socket.h"
#include "extern.h"
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

struct iSupport
{
  // Other options the server might send.
  Flux::map<Flux::string> other;
  // Supported chan modes
  Flux::string ChanModes;
  // Supported chan types
  Flux::string ChanTypes;
  // The IRCd version
  Flux::string IRCdVersion;
  // User modes
  Flux::string UserModes;
  // Network name
  Flux::string Network;
  // Servers hostname
  Flux::string ServerHost;
  // Max away len
  int AwayLen;
  // Max kick len
  int KickLen;
  // Max channels you can join
  int MaxChannels;
  // Max number of bans settable
  int MaxBans;
  // Max nickname length
  int NickLen;
  // Max Topic Length
  int TopicLen;
};

class Network : public Timer
{
protected:
  bool disconnecting;
  Flux::string usedhostname;
public:
  Network(const Flux::string&, const Flux::string&, const Flux::string &n = "");
  ~Network();
  // The socket
  NetworkSocket *s;
  // What that network supports
  iSupport isupport;
  // When we join a network but aren't synced yet
  std::queue<Channel*> JoinQueue;
  // bot pointer for the network
  Bot *b;
  // Map of all users in the network
  Flux::insensitive_map<User*> UserNickList;
  // Map of all channels in the network
  Flux::insensitive_map<Channel*> ChanMap;
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

  void Tick(time_t);
  void Sync();
  void SetConnectedHostname(const Flux::string &str) { this->usedhostname = str; }
  Flux::string GetConHost() const { return this->usedhostname; }
  bool JoinChannel(const Flux::string&);
  bool IsDisconnecting() { return this->disconnecting; }
  bool Disconnect();
  bool Disconnect(const char *fmt, ...);
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

/* Base socket class for ALL network connections for the Network class */
/* This isnt in network.h because of some include recursion issues -_- */
class NetworkSocket : public ConnectionSocket, public BufferedSocket
{
public:
  NetworkSocket(Network*);
  ~NetworkSocket();
  Network *net;
  bool SentPing;
  bool Read(const Flux::string&);
  bool ProcessWrite();
  void OnConnect();
  void OnError(const Flux::string&);
};

// NetworkSocket is in socket.h :/
#endif