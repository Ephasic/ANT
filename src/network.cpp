#include "network.h"

Network::Network(const Flux::string &host, const Flux::string &p, const Flux::string &n): name(n), hostname(host), port(p)
{
  Log(LOG_DEBUG) << "New network created: " << n << " " << host << ':' << p;
}

bool Network::Disconnect() { return false; }
bool Network::Connect() { return false; }