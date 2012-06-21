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

class PingTimer : public Timer
{
public:
  PingTimer():Timer(60, time(NULL), true) { }
  void Tick(time_t)
  {
    for(auto it : Networks)
    {
      Network *n = it.second;
      if(n->IsSynced() && n->b)
      {
// 	if(n->s->SentPing)
// 	{
// 	  n->s->SetDead(true);
// 	  Log(LOG_DEBUG) << n->name << " timed out, reconnecting.";
// 	}
// 	else
// 	{
	  #ifdef HAVE_GETTIMEOFDAY
	  struct timeval tv;
	  gettimeofday(&tv, NULL);

	  // Milisecond lag times :D
	  n->b->ircproto->ping("%i.%06d", tv.tv_sec, static_cast<int>(tv.tv_usec));
	  #else
	  n->b->ircproto->ping("%i", static_cast<int>(time(NULL)));
	  #endif
	  n->s->pings++;

// 	}
      }
    }
  }
};

class Ping_pong : public Module
{
  PingTimer pingtimer;
public:
  Ping_pong(const Flux::string &Name):Module(Name, MOD_NORMAL)
  {
    Implementation i[] = { I_OnPong, I_OnPing };
    ModuleHandler::Attach(i, this, sizeof(i) / sizeof(Implementation));
    this->SetAuthor("Justasic");
    this->SetVersion(VERSION);
    this->SetPriority(PRIORITY_FIRST);
  }
  
  void OnPong(const std::vector<Flux::string> &params, Network *n)
  {
    n->s->pings = 0;
#ifdef HAVE_GETTIMEOFDAY
    Flux::string timestamp = params[1].substr(0, params[1].find('.'));
    Flux::string miliseconds = params[1].substr(params[1].find('.')+1);

    struct timeval tv;
    gettimeofday(&tv, NULL);

    int secondlag = tv.tv_sec - static_cast<int>(timestamp);
    int milisecond = tv.tv_usec - static_cast<int>(miliseconds);
    Log(LOG_RAWIO) << secondlag << '.' << milisecond << " sec lag (" << params[1] << " - " << tv.tv_sec << '.' << tv.tv_usec << ")";
#else
    Flux::string ts = params[1];
    int lag = time(NULL) - static_cast<int>(ts);

    Log(LOG_RAWIO) << lag << " sec lag (" << ts << " - " << time(NULL) << ')';
#endif
  }
  
  void OnPing(const std::vector<Flux::string> &params, Network *n)
  {
    n->s->pings = 0;
    n->s->Write("PONG :%s\n", params[0].c_str());
  }
};

MODULE_HOOK(Ping_pong)
