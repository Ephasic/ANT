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
#pragma once
#ifndef TQUEUE_H
#define TQUEUE_H
#include "network.h"

/*
 * This is some kind of timed callback system, this
 * class will take a void function and store it in a
 * map, then a timer will call the function after a
 * certain time and process what that function needed
 *
 * This is useful for temporary timers and gets rid of
 * having to make hundreds of timers for different little
 * things
 */
extern std::vector<tqueue*> QueuedQueues;

class tqueue : public Timer
{
    // Function to call.
    void (*function)();

    // Timer to call the function
    void Tick(time_t);
public:
    tqueue(void (*func)(), long time_to_tick, time_t now = time(NULL), bool repeating = false);
    virtual ~tqueue();
};

#endif