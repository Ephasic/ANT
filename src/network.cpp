#include "network.h"

Network::Network(const Flux::string &host, const Flux::string &p, const Flux::string &n = ""): hostname(host), port(p), name(n)
{
  Log(LOG_DEBUG) << "New network created: " << n << " " << host << ':' << p;
}