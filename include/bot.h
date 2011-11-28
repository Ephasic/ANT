#ifndef _BOT_H
#define _BOT_H
#include "module.h"
#include "network.h"

class CoreExport Bot
{
public:
  Bot(Network *net, const Flux::string &n, const Flux::string &i, const Flux::string &real);
  /* The network we're on*/
  Network *network;
  /* List of chans the bot is in */
  Flux::map<Channel*> channels;
  /* Bot's current nickname */
  Flux::string nick;
  /* Bot's current ident */
  Flux::string ident;
  /* Bot's current realname */
  Flux::string realname;
  void AnnounceCommit(CommitMessage&);
  
};

#endif