#include "bot.h"

Bot::Bot(Network *net, const Flux::string &n, const Flux::string &i, const Flux::string &real): network(net), nick(n), ident(i), realname(real)
{
  Log(LOG_DEBUG) << "New bot created on " << net->name << ": " << n << i << ": " << real;
}

void Bot::AnnounceCommit(CommitMessage &msg)
{
}