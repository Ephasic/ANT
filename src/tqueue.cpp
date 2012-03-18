/* Arbitrary Navn Tool -- Timed Callback Queue
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

#include "tqueue.h"
#include "log.h"

std::vector<tqueue*> QueuedQueues;

tqueue::tqueue(void (*func)(), long time_to_tick, time_t now, bool repeating) : Timer(time_to_tick, now, repeating), function(func)
{
  QueuedQueues.push_back(this);
  Log(LOG_TERMINAL) << "Creating queue: @" << this;
}

tqueue::~tqueue()
{
  for(std::vector<tqueue*>::iterator it = QueuedQueues.begin(); it != QueuedQueues.end(); ++it)
    if(*it == this)
      QueuedQueues.erase(it);
      
  Log(LOG_TERMINAL) << "Destroying queue: @" << this;
}

void tqueue::Tick(time_t)
{
  Log(LOG_TERMINAL) << "Running queue: @" << this;
  void (*runfunc)() = this->function;
  runfunc();
}