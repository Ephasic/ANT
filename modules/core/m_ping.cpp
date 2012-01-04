/* Arbitrary Navn Tool -- IRC Ping Handler Module
 * 
 * (C) 2011-2012 Flux-Net
 * Contact us at Dev@Flux-Net.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#include "flux_net_irc.hpp"

class WaitTimer:public Timer
{
public:
  WaitTimer():Timer(120, time(NULL)) {}
  void Tick(time_t)
  {
    Log(LOG_RAWIO) << "Wait Timer Tick.";
    for(auto it : Networks){
      JoinChansInBuffer(it.second);
      if(it.second->s && it.second->s->IsConnected() && it.second->s->SentPing)
	it.second->s->SetDead(true); // Ping Timeout, let the SocketEngine class handle this :P
    }
  }
};

class PingTimer:public Timer
{
public:
  PingTimer():Timer(60, time(NULL), true) { }
  void Tick(time_t){
    Send_Global("PING :%i\n", static_cast<int>(time(NULL)));
    new WaitTimer();
  }
};

class Ping_pong:public module
{
  PingTimer pingtimer;
public:
  Ping_pong(const Flux::string &Name):module(Name)
  {
    Implementation i[] = { I_OnPong, I_OnPing, I_OnPostConnect };
    ModuleHandler::Attach(i, this, sizeof(i) / sizeof(Implementation));
    this->SetAuthor("Justasic");
    this->SetVersion(VERSION);
    this->SetPriority(PRIORITY_FIRST);
  }
  void OnPong(const std::vector<Flux::string> &params, Network *n)
  {
     Flux::string ts = params[1];
     int lag = time(NULL)-(int)ts;
     n->s->SentPing = false;
     if(protocoldebug)
        Log(LOG_RAWIO) << lag << " sec lag (" << ts << " - " << time(NULL) << ')';
  }
  void OnPing(const std::vector<Flux::string> &params, Network *n)
  {
    n->s->SentPing = false;
    n->s->Write("PONG :%s\n", params[0].c_str());
  }
};
MODULE_HOOK(Ping_pong)
