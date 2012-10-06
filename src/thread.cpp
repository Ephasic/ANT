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
    thread->Execute();
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
    pthread_mutex_init(&this->mutex, NULL);
}

Mutex::~Mutex()
{
    pthread_mutex_destroy(&this->mutex);
}

void Mutex::Lock()
{
    pthread_mutex_lock(&this->mutex);
}

void Mutex::Unlock()
{
    pthread_mutex_unlock(&this->mutex);
}

bool Mutex::TryLock()
{
    return pthread_mutex_trylock(&mutex) == 0;
}

Condition::Condition() : Mutex()
{
    pthread_cond_init(&this->cond, NULL);
}

Condition::~Condition()
{
    pthread_cond_destroy(&this->cond);
}

void Condition::Wakeup()
{
    pthread_cond_signal(&this->cond);
}

void Condition::Wait()
{
    pthread_cond_wait(&this->cond, &this->mutex);
}
