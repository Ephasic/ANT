#ifndef _BOT_H
#define _BOT_H
#include "user.h"
#include "network.h"

class CoreExport Bot : public User
{
public:
  Bot(Network *net, const Flux::string &n, const Flux::string &i, const Flux::string &real = "ANT Bot (http://ANT.Flux-Net.net/)");
  ~Bot();
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
  /* IRCProto class for the network */
  IRCProto *ircproto;
  /* Announce a commit that has been made */
  void AnnounceCommit(CommitMessage&);
  /* Join a channel */
  void Join(Channel*);
  void Join(const Flux::string&);
  /* Part a channel */
  void Part(Channel*, const Flux::string &msg = "");
  /* Quit the IRC network */
  void Quit(const Flux::string &msg = "");
  /* Change a nickname */
  void SetNick(const Flux::string&);
  /* Send the user credentials for connecting */
  void SendUser();
};

#endif