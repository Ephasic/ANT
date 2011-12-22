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
  /* Announce a commit that has been made */
  void AnnounceCommit(CommitMessage&);
  /* Join a channel */
  void Join(Channel*);
  /* Part a channel */
  void Part(Channel*);
  /* Quit the IRC network */
  void Quit();
  /* Change a nickname */
  void SetNick(const Flux::string&);
  /* Send the user credentials for connecting */
  void SendUser();
  
};

#endif