#include "Socket.h"

static int MaxFD;
static unsigned FDCount;
static fd_set ReadFDs;
static fd_set WriteFDs;

void SocketEngine::Init()
{
  MaxFD = 0;
  FDCount = 0;
  FD_ZERO(&ReadFDs);
  FD_ZERO(&WriteFDs);
}

void SocketEngine::Shutdown()
{
  Process();

  for (std::map<int, Socket*>::const_iterator it = Sockets.begin(), it_end = Sockets.end(); it != it_end;)
  {
    Socket *s = it->second;
    ++it;
    delete s;
  }
  Sockets.clear();
}

void SocketEngine::AddSocket(Socket *s)
{
  if (s->GetFD() > MaxFD)
    MaxFD = s->GetFD();
  FD_SET(s->GetFD(), &ReadFDs);
  Sockets.insert(std::make_pair(s->GetFD(), s));
  ++FDCount;
}

void SocketEngine::DelSocket(Socket *s)
{
  if (s->GetFD() == MaxFD)
    --MaxFD;
  FD_CLR(s->GetFD(), &ReadFDs);
  FD_CLR(s->GetFD(), &WriteFDs);
  Sockets.erase(s->GetFD());
  --FDCount;
}

void SocketEngine::MarkWritable(Socket *s)
{
  if (s->IsWritable())
    return;
  FD_SET(s->GetFD(), &WriteFDs);
  s->SetWritable(true);
}

void SocketEngine::ClearWritable(Socket *s)
{
  if (!s->IsWritable())
    return;
  FD_CLR(s->GetFD(), &WriteFDs);
  s->SetWritable(false);
}

void SocketEngine::Process()
{
  fd_set rfdset = ReadFDs, wfdset = WriteFDs, efdset = ReadFDs;
  timeval tval;
  tval.tv_sec = Config->SockWait;
  tval.tv_usec = 0;

  #ifdef _WIN32
  /* We can use the socket engine to "sleep" services for a period of
   * time between connections to the uplink, which allows modules,
   * timers, etc to function properly. Windows, being as useful as it is,
   * does not allow to select() on 0 sockets and will immediately return error.
   * Thus:
   */
  if (FDCount == 0)
  {
    sleep(tval.tv_sec);
    return;
  }
  #endif

  int sresult = select(MaxFD + 1, &rfdset, &wfdset, &efdset, &tval);

  if (sresult == -1)
  {
    Log() << "SockEngine::Process(): error: " << strerror(errno);
  }
  else if (sresult)
  {
    int processed = 0;
    for (auto it = Sockets.begin(), it_end = Sockets.end(); it != it_end && processed != sresult;)
    {
      Socket *s = it->second;
      ++it;
      Log(LOG_TERMINAL) << "Processing Socket " << s->GetFD();
      bool has_read = FD_ISSET(s->GetFD(), &rfdset), has_write = FD_ISSET(s->GetFD(), &wfdset), has_error = FD_ISSET(s->GetFD(), &efdset);
      if (has_read || has_write || has_error)
	++processed;
      if (has_error)
      {
	s->ProcessError();
	s->SetDead(true);
	delete s;
	continue;
      }

      if (!s->Process())
	continue;

      if (has_read && !s->ProcessRead()){
 	s->SetDead(true);
	Log(LOG_TERMINAL) << "Socket " << s->GetFD() << " set dead due to no read data! | " << (has_read?"Has Read": "No Read") << (s->ProcessRead()?"| READ DATA!":" | KEIN READ DATA!");
      }

      if (has_write && !s->ProcessWrite())
	s->SetDead(true);
      
      if (s->IsDead()){
	Log(LOG_TERMINAL) << "Socket " << s->GetFD() << " died!";
	delete s;
      }
    }
  }
}