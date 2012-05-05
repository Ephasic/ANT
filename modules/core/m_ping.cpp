/* Arbitrary Navn Tool -- IRC Ping Handler Module
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#include "modules.h"

class PingTimer:public Timer
{
public:
  PingTimer():Timer(60, time(NULL), true) { }
  void Tick(time_t)
  {
    for(auto it : Networks)
    {
      Network *n = it.second;
      if(n->b && n->b->ircproto)
      {
	n->b->ircproto->Raw("PING :%i", static_cast<int>(time(NULL)));
	n->s->SentPing = true;
      }
    }
  }
};

class Ping_pong : public module
{
  PingTimer pingtimer;
public:
  Ping_pong(const Flux::string &Name):module(Name)
  {
//     new PingTimer();
    Implementation i[] = { I_OnPong, I_OnPing };
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
