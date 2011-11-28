#ifndef _NETWORK_H_
#define _NETWORK_H_
#include "module.h"
#include "bots.h"

class Network;
struct CommitMessage
{
  std::vector<Flux::string> MessageMeta;
  std::vector<Bot*> Bots;
  Network *network;
}

class CoreExport Network
{};

#endif