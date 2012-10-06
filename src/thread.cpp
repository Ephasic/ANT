/* Arbitrary Navn Tool -- Threading Engine
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

#include "thread.h"
static inline pthread_attr_t *GetAttr()
{
    static pthread_attr_t attr;

    if(pthread_attr_init(&attr))
	throw CoreException("Error calling pthread_attr_init for Threads");
    if(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE))
	throw CoreException("Unable to make threads joinable");

    return &attr;
}

Thread::Thread():exit(false) {}

Thread::~Thread() { }

void Thread::Notify() {}

void *EntryPoint(void *parameter)
{
    Thread *thread = static_cast<Thread*>(parameter);
    thread->ToRun();
    thread->SetExitState();
    pthread_exit(0);
}

void Thread::SetExitState()
{
    this->Notify();
    exit = true;
}

bool Thread::GetExitState() const { return exit; }
void Thread::OnNotify() { this->Join(); }
void Thread::Start()
{
    if(pthread_create(&this->Handle, GetAttr(), EntryPoint, this))
    {
	throw CoreException("Could not Create Thread: "+value_cast<Flux::string>(strerror(errno)));
    }
}

void Thread::Join()
{
    this->SetExitState();
    pthread_join(Handle, NULL);
}

void Thread::Exit()
{
  this->SetExitState();
  pthread_exit(0);
}

Mutex::Mutex()
{
    pthread_mutex_init(&mutex, NULL);
}

Mutex::~Mutex()
{
    pthread_mutex_destroy(&mutex);
}

void Lock()
{
    pthread_mutex_lock(&mutex);
}

void Unlock()
{
    pthread_mutex_unlock(&mutex);
}

bool TryLock()
{
    return pthread_mutex_trylock(&mutex) == 0;
}

Condition::Condition() : Mutex()
{
    pthread_cond_init(&cond);
}

Condition::~Condition()
{
    pthread_cond_destroy(&cond);
}

void Condition::Wakeup()
{
    pthread_cond_signal(&cond);
}

void Condition::Wait()
{
    pthread_cond_wait(&cond, &mutex);
}
