/* Arbitrary Navn Tool -- Threading Engine Prototype
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
#ifndef THREAD_H
#define THREAD_H
#include "flux.h"
#include "SocketException.h"
#include <errno.h>
#include <pthread.h>

class Thread : public Base
{
  pthread_t Handle;
  bool exit;
public:
  /* Anope: "Exit the thread. Note that the thread still must be joined to free resources!" */
  Thread();
  virtual ~Thread();
  void SetExitState();
  void OnNotify();
  void Notify();
  bool GetExitState() const;
  void Join();
  void Exit();
  void Start();
  virtual void ToRun() =0;
};
#endif