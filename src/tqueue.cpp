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

// Queues :D
std::vector<tqueue*> QueuedQueues;

// Our constructor where we create our timer and push
tqueue::tqueue(void (*func)(), long time_to_tick, time_t now, bool repeating) : Timer(time_to_tick, now, repeating), function(func)
{
  QueuedQueues.push_back(this);
  Log(LOG_MEMORY) << "Creating queue: @" << this;
}

// Destroy our queue
tqueue::~tqueue()
{
  std::vector<tqueue*>::iterator it = std::find(QueuedQueues.begin(), QueuedQueues.end(), this);
  if(it != QueuedQueues.end())
      QueuedQueues.erase(it);

  Log(LOG_MEMORY) << "Destroying queue: @" << this;
}

// Call our callback
void tqueue::Tick(time_t)
{
  Log(LOG_MEMORY) << "Running queue: @" << this;
  void (*runfunc)() = this->function;
  runfunc();
}