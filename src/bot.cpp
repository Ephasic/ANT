#include "bot.h"

Bot::Bot(Network *net, const Flux::string &n, const Flux::string &i, const Flux::string &real): network(net), nick(n), ident(i), realname(real)
{
  Log(LOG_DEBUG) << "New bot created on " << net->name << ": " << n << i << ": " << real;
}

void Bot::AnnounceCommit(CommitMessage &msg)
{
  for(unsigned i=0; i < msg.MessageMeta.size(); ++i)
  {
    for(auto it : this->channels)
    {
      Channel *c = it->second;
      if(c != NULL)
      {
	printf("I would process the channel for commiting.. this needs work!\n");
      }
    }
  }
}