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

// // Our constructor where we create our timer and push
// tqueue::tqueue(void (*func)(), long time_to_tick, time_t now, bool repeating) : Timer(time_to_tick, now, repeating), function(func)
// {
//   QueuedQueues.push_back(this);
//   Log(LOG_TERMINAL) << "Creating queue: @" << this;
// }
// 
// // Our constructor where we create our timer with a pointer
// tqueue::tqueue(void (*func)(void*), long time_to_tick, time_t now, bool repeating) : Timer(time_to_tick, now, repeating), function(func)
// {
//   QueuedQueues.push_back(this);
//   Log(LOG_TERMINAL) << "Creating queue: @" << this;
// }
// 
// // Our constructor where we create our timer with 2 pointers
// tqueue::tqueue(void (*func)(void*, void*), long time_to_tick, time_t now, bool repeating) : Timer(time_to_tick, now, repeating), function(func)
// {
//   QueuedQueues.push_back(this);
//   Log(LOG_TERMINAL) << "Creating queue: @" << this;
// }
// 
// // Our constructor where we create our timer with an infinite number of arguments using va_list
// tqueue::tqueue(void (*func)(...), long time_to_tick, time_t now, bool repeating) : Timer(time_to_tick, now, repeating), function(func)
// {
//   QueuedQueues.push_back(this);
//   Log(LOG_TERMINAL) << "Creating queue: @" << this;
// }
template<typename... params2>
tqueue::tqueue(void (*func)(params2...), long time_to_tick, time_t now, bool repeating) : Timer(time_to_tick, now, repeating), function(func)
{
  QueuedQueues.push_back(this);
  this->params = &parms;
  Log(LOG_TERMINAL) << "Creating queue: @" << this;
}

// Destroy our queue
tqueue::~tqueue()
{
  std::vector<tqueue*>::iterator it = std::find(QueuedQueues.begin(), QueuedQueues.end(), this);
  if(it != QueuedQueues.end())
      QueuedQueues.erase(it);
      
  Log(LOG_TERMINAL) << "Destroying queue: @" << this;
}

void tqueue::Tick(time_t)
{
  Log(LOG_TERMINAL) << "Running queue: @" << this;
  template<typename... funcparams> void (*runfunc)(funcparams...) = this->function;
  runfunc(*this->params);
}

// // Call our callback
// void tqueue::Tick(time_t)
// {
//   Log(LOG_TERMINAL) << "Running queue: @" << this;
//   void (*runfunc)() = this->function;
//   runfunc();
// }