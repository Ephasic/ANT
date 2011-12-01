/*
 * This is the module file for the XML_RPC commits, this will handle 98% of the commits the bot will process!
 */
#include "flux_net_irc.hpp"
// class xmlrpcsock : public ListenSocket
// {
//   xmlrpcsock(const Flux::string &ip, int port, bool ipv6):ListenSocket(ip, port, ipv6){}
//   virtual ~xmlrpcsock() {}
//   virtual ClientSocket *OnAccept(int fd, const sockaddrs &addr) = 0;
// };
class xmlrpcmod : public module
{
public:
  xmlrpcmod(const Flux::string &Name):module(Name)
  {
    this->SetAuthor("Justasic");
    this->SetVersion(VERSION);
    Implementation i[] = { I_OnCommit };
    ModuleHandler::Attach(i, this, sizeof(i)/sizeof(Implementation));
  }
  void OnCommit(CommitMessage &msg)
  {
    Log(LOG_TERMINAL) << "Fun stuff!";
  }
};