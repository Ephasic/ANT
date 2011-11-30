/* Socket.cpp */
/* All code is licensed under GNU General Public License GPL v3 (http://www.gnu.org/licenses/gpl.html) */
/**
 *\file  Socket.cpp 
 *\brief Contains the Socket engine.
 */
#include "SocketException.h"
#include "Socket.h"
#include <fcntl.h>
#include <iostream>

std::map<int, Socket *> SocketEngine::Sockets;

int32_t TotalRead = 0;
int32_t TotalWritten = 0;

SocketIO normalSocketIO;

/** Construct the object, sets everything to 0
 */
sockaddrs::sockaddrs()
{
  this->clear();
}

/** Memset the object to 0
 */
void sockaddrs::clear()
{
  memset(this, 0, sizeof(*this));
}

/** Get the size of the sockaddr we represent
 * @return The size
 */
size_t sockaddrs::size() const
{
  switch (sa.sa_family)
  {
    case AF_INET:
      return sizeof(sa4);
    case AF_INET6:
      return sizeof(sa6);
    default:
      break;
  }
  
  return 0;
}

/** Get the port represented by this addr
 * @return The port, or -1 on fail
 */
int sockaddrs::port() const
{
  switch (sa.sa_family)
  {
    case AF_INET:
      return ntohs(sa4.sin_port);
    case AF_INET6:
      return ntohs(sa6.sin6_port);
    default:
      break;
  }
  
  return -1;
}

/** Get the address represented by this addr
 * @return The address
 */
Flux::string sockaddrs::addr() const
{
  char address[INET6_ADDRSTRLEN + 1] = "";
  
  switch (sa.sa_family)
  {
    case AF_INET:
      if (!inet_ntop(AF_INET, &sa4.sin_addr, address, sizeof(address)))
	throw SocketException(strerror(errno));
      return address;
    case AF_INET6:
      if (!inet_ntop(AF_INET6, &sa6.sin6_addr, address, sizeof(address)))
	throw SocketException(strerror(errno));
      return address;
    default:
      break;
  }
  
  return address;
}

/** Check if this sockaddr has data in it
 */
bool sockaddrs::operator()() const
{
  return this->sa.sa_family != 0;
}

/** Compares with sockaddr with another. Compares address type, port, and address
 * @return true if they are the same
 */
bool sockaddrs::operator==(const sockaddrs &other) const
{
  if (sa.sa_family != other.sa.sa_family)
    return false;
  switch (sa.sa_family)
  {
    case AF_INET:
      return (sa4.sin_port == other.sa4.sin_port) && (sa4.sin_addr.s_addr == other.sa4.sin_addr.s_addr);
    case AF_INET6:
      return (sa6.sin6_port == other.sa6.sin6_port) && !memcmp(sa6.sin6_addr.s6_addr, other.sa6.sin6_addr.s6_addr, 16);
    default:
      return !memcmp(this, &other, sizeof(*this));
  }
  
  return false;
}

/** The equivalent of inet_pton
 * @param type AF_INET or AF_INET6
 * @param address The address to place in the sockaddr structures
 * @param pport An option port to include in the  sockaddr structures
 * @throws A socket exception if given invalid IPs
 */
void sockaddrs::pton(int type, const Flux::string &address, int pport)
{
  switch (type)
  {
    case AF_INET:
    {
      int i = inet_pton(type, address.c_str(), &sa4.sin_addr);
      if (i == 0)
	throw SocketException("Invalid host");
      else if (i <= -1)
	throw SocketException(fsprintf("Invalid host: %s", strerror(errno)));
      sa4.sin_family = type;
      sa4.sin_port = htons(pport);
      return;
    }
    case AF_INET6:
    {
      int i = inet_pton(type, address.c_str(), &sa6.sin6_addr);
      if (i == 0)
	throw SocketException("Invalid host");
      else if (i <= -1)
	throw SocketException(fsprintf("Invalid host: %s", strerror(errno)));
      sa6.sin6_family = type;
      sa6.sin6_port = htons(pport);
      return;
    }
    default:
      break;
  }
  
  throw CoreException("Invalid socket type");
}

/** The equivalent of inet_ntop
 * @param type AF_INET or AF_INET6
 * @param address The in_addr or in_addr6 structure
 * @throws A socket exception if given an invalid structure
 */
void sockaddrs::ntop(int type, const void *src)
{
  switch (type)
  {
    case AF_INET:
      sa4.sin_addr = *reinterpret_cast<const in_addr *>(src);
      sa4.sin_family = type;
      return;
    case AF_INET6:
      sa6.sin6_addr = *reinterpret_cast<const in6_addr *>(src);
      sa6.sin6_family = type;
      return;
    default:
      break;
  }
  
  throw CoreException("Invalid socket type");
}

cidr::cidr(const Flux::string &ip)
{
  if (ip.find_first_not_of("01234567890:./") != Flux::string::npos)
    throw SocketException("Invalid IP");
  
  bool ipv6 = ip.find(':') != Flux::string::npos;
  size_t sl = ip.find_last_of('/');
  if (sl == Flux::string::npos)
  {
    this->cidr_ip = ip;
    this->cidr_len = ipv6 ? 128 : 32;
    this->addr.pton(ipv6 ? AF_INET6 : AF_INET, ip);
  }
  else
  {
    Flux::string real_ip = ip.substr(0, sl);
    Flux::string cidr_range = ip.substr(sl + 1);
    if (!cidr_range.is_pos_number_only())
      throw SocketException("Invalid CIDR range");
    
    this->cidr_ip = real_ip;
    this->cidr_len = value_cast<unsigned int>(cidr_range);
    this->addr.pton(ipv6 ? AF_INET6 : AF_INET, real_ip);
  }
}

cidr::cidr(const Flux::string &ip, unsigned char len)
{
  bool ipv6 = ip.find(':') != Flux::string::npos;
  this->addr.pton(ipv6 ? AF_INET6 : AF_INET, ip);
  this->cidr_ip = ip;
  this->cidr_len = len;
}

Flux::string cidr::mask() const
{
  return this->cidr_ip + "/" + this->cidr_len;
}

bool cidr::match(sockaddrs &other)
{
  if (this->addr.sa.sa_family != other.sa.sa_family)
    return false;
  
  unsigned char *ip, *their_ip, byte;
  
  switch (this->addr.sa.sa_family)
  {
    case AF_INET:
      ip = reinterpret_cast<unsigned char *>(&this->addr.sa4.sin_addr);
      byte = this->cidr_len / 8;
      their_ip = reinterpret_cast<unsigned char *>(&other.sa4.sin_addr);
      break;
    case AF_INET6:
      ip = reinterpret_cast<unsigned char *>(&this->addr.sa6.sin6_addr);
      byte = this->cidr_len / 8;
      their_ip = reinterpret_cast<unsigned char *>(&other.sa6.sin6_addr);
      break;
    default:
      throw SocketException("Invalid address type");
  }
  
  if (memcmp(ip, their_ip, byte))
    return false;
  
  ip += byte;
  their_ip += byte;
  byte = this->cidr_len % 8;
  if ((*ip & byte) != (*their_ip & byte))
    return false;
  
  return true;
}

/** Receive something from the buffer
 * @param s The socket
 * @param buf The buf to read to
 * @param sz How much to read
 * @return Number of bytes received
 */
int SocketIO::Recv(Socket *s, char *buf, size_t sz)
{
  size_t i = recv(s->GetFD(), buf, sz, 0);
  TotalRead += i;
  return i;
}

/** Write something to the socket
 * @param s The socket
 * @param buf The data to write
 * @param size The length of the data
 */
int SocketIO::Send(Socket *s, const char *buf, size_t sz)
{
  size_t i = send(s->GetFD(), buf, sz, 0);
  TotalWritten += i;
  return i;
}
int SocketIO::Send(Socket *s, const Flux::string &buf)
{
  return this->Send(s, buf.c_str(), buf.length());
}

/** Accept a connection from a socket
 * @param s The socket
 * @return The new client socket
 */
ClientSocket *SocketIO::Accept(ListenSocket *s)
{
  sockaddrs conaddr;
  
  socklen_t size = sizeof(conaddr);
  int newsock = accept(s->GetFD(), &conaddr.sa, &size);
  
  if (newsock >= 0)
  {
    ClientSocket *ns = s->OnAccept(newsock, conaddr);
    ns->SetAccepted(true);
    ns->OnAccept();
    return ns;
  }
  else
    throw SocketException(fsprintf("Unable to accept connection: %s", strerror(errno)));
}

/** Finished accepting a connection from a socket
 * @param s The socket
 * @return SF_ACCEPTED if accepted, SF_ACCEPTING if still in process, SF_DEAD on error
 */
SocketFlag SocketIO::FinishAccept(ClientSocket *cs)
{
  return SF_ACCEPTED;
}

/** Bind a socket
 * @param s The socket
 * @param ip The IP to bind to
 * @param port The optional port to bind to
 */
void SocketIO::Bind(Socket *s, const Flux::string &ip, int port)
{
  s->bindaddr.pton(s->IsIPv6() ? AF_INET6 : AF_INET, ip, port);
  if (bind(s->GetFD(), &s->bindaddr.sa, s->bindaddr.size()) == -1)
    throw SocketException(fsprintf("Unable to bind to address: %s", strerror(errno)));
}

/** Connect the socket
 * @param s THe socket
 * @param target IP to connect to
 * @param port to connect to
 */
void SocketIO::Connect(ConnectionSocket *s, const Flux::string &target, int port)
{
  s->SetConnected(false);
  s->SetConnecting(false);
  s->conaddr.pton(s->IsIPv6() ? AF_INET6 : AF_INET, target, port);
  int c = connect(s->GetFD(), &s->conaddr.sa, s->conaddr.size());
  if (c == -1)
  {
    if (errno != EINPROGRESS)
      s->OnError(strerror(errno));
    else
    {
      SocketEngine::MarkWritable(s);
      s->SetConnecting(true);
    }
  }
  else
  {
    s->SetConnected(true);
    s->OnConnect();
  }
}

/** Called to potentially finish a pending connection
 * @param s The socket
 * @return SF_CONNECTED on success, SF_CONNECTING if still pending, and SF_DEAD on error.
 */
SocketFlag SocketIO::FinishConnect(ConnectionSocket *s)
{
  if (s->IsConnected())
    return SF_CONNECTED;
  else if (!s->IsConnecting())
    throw SocketException("SocketIO::FinishConnect called for a socket not connected nor connecting?");
  
  int optval = 0;
  socklen_t optlen = sizeof(optval);
  if (!getsockopt(s->GetFD(), SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&optval), &optlen) && !optval)
  {
    s->SetConnected(true);
    s->SetConnecting(false);
    s->OnConnect();
    return SF_CONNECTED;
  }
  else
  {
    errno = optval;
    s->OnError(optval ? strerror(errno) : "");
    return SF_DEAD;
  }
}

/** Empty constructor, should not be called.
 */
Socket::Socket()
{
  throw CoreException("Socket::Socket() ?");
}

/** Constructor
 * @param sock The socket
 * @param ipv6 IPv6?
 * @param type The socket type, defaults to SOCK_STREAM
 */
Socket::Socket(int sock, bool ipv6, int type)
{
  this->IO = &normalSocketIO;
  this->IPv6 = ipv6;
  if (sock == -1)
    this->Sock = socket(this->IPv6 ? AF_INET6 : AF_INET, type, 0);
  else
    this->Sock = sock;
  this->SetNonBlocking();
  SocketEngine::AddSocket(this);
}

/** Default destructor
 */
Socket::~Socket()
{
  SocketEngine::DelSocket(this);
  close(this->Sock);
  this->IO->Destroy();
}

/** Get the socket FD for this socket
 * @return the fd
 */
int Socket::GetFD() const
{
  return Sock;
}

/** Check if this socket is IPv6
 * @return true or false
 */
bool Socket::IsIPv6() const
{
  return IPv6;
}

/** Set the socket as being dead
 * @param bool boolean if the socket is dead
 */
void Socket::SetDead(bool dead)
{
  isdead = dead;
}

/** Get socket status if it is dead or not
 * @return bool boolean if the socket is dead
 */
bool Socket::IsDead()
{
  return isdead;
}

void Socket::SetConnecting(bool s)
{
  isconnecting = s;
}

bool Socket::IsConnecting()
{
  return isconnecting;
}

void Socket::SetConnected(bool s)
{
  isconnected = s;
}

bool Socket::IsConnected()
{
  return isconnected;
}

void Socket::SetAccepting(bool s)
{
  isaccepting = s;
}

bool Socket::IsAccepting()
{
  return isaccepting;
}

void Socket::SetAccepted(bool s)
{
  isaccepted = s;
}

bool Socket::IsAccepted()
{
  return isaccepted;
}

void Socket::SetWritable(bool s)
{
  iswritable = s;
}

bool Socket::IsWritable()
{
  return iswritable;
}

void Socket::SetStatus(SocketFlag s)
{
  switch(s)
  {
    case SF_WRITABLE:
      this->SetWritable(true);
      break;
    case SF_ACCEPTED:
      this->SetAccepted(true);
      break;
    case SF_ACCEPTING:
      this->SetAccepting(true);
      break;
    case SF_CONNECTED:
      this->SetConnected(true);
      break;
    case SF_CONNECTING:
      this->SetConnecting(true);
      break;
    case SF_DEAD:
      this->SetDead(true);
      break;
  }
}

/** Mark a socket as blockig
 * @return true if the socket is now blocking
 */
bool Socket::SetBlocking()
{
  int flags = fcntl(this->GetFD(), F_GETFL, 0);
  return !fcntl(this->GetFD(), F_SETFL, flags & ~O_NONBLOCK);
}

/** Mark a socket as non-blocking
 * @return true if the socket is now non-blocking
 */
bool Socket::SetNonBlocking()
{
  int flags = fcntl(this->GetFD(), F_GETFL, 0);
  return !fcntl(this->GetFD(), F_SETFL, flags | O_NONBLOCK);
}

/** Bind the socket to an ip and port
 * @param ip The ip
 * @param port The port
 */
void Socket::Bind(const Flux::string &ip, int port)
{
  this->IO->Bind(this, ip, port);
}

/** Called when there either is a read or write event.
 * @return true to continue to call ProcessRead/ProcessWrite, false to not continue
 */
bool Socket::Process()
{
  return true;
}

/** Called when there is something to be received for this socket
 * @return true on success, false to drop this socket
 */
bool Socket::ProcessRead()
{
  return true;
}

/** Called when the socket is ready to be written to
 * @return true on success, false to drop this socket
 */
bool Socket::ProcessWrite()
{
  return true;
}

/** Called when there is an error for this socket
 * @return true on success, false to drop this socket
 */
void Socket::ProcessError()
{
}

/** Constructor
 * @param bindip The IP to bind to
 * @param port The port to listen on
 * @param ipv6 true for ipv6
 */
ListenSocket::ListenSocket(const Flux::string &bindip, int port, bool ipv6) : Socket(-1, ipv6)
{
  this->SetNonBlocking();
  
  const char op = 1;
  setsockopt(this->GetFD(), SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op));
  
  this->bindaddr.pton(IPv6 ? AF_INET6 : AF_INET, bindip, port);
  this->IO->Bind(this, bindip, port);
  
  if (listen(Sock, SOMAXCONN) == -1)
    throw SocketException(fsprintf("Unable to listen: %s", strerror(errno)));
}

/** Destructor
 */
ListenSocket::~ListenSocket()
{
}

/** Accept a connection in this sockets queue
 */
bool ListenSocket::ProcessRead()
{
  try
  {
    this->IO->Accept(this);
  }
  catch (const SocketException &ex)
  {
    Log() << ex.GetReason();
  }
  return true;
}

BufferedSocket::BufferedSocket()
{
}

BufferedSocket::~BufferedSocket()
{
}

bool BufferedSocket::ProcessRead()
{
  char tbuffer[NET_BUFSIZE];
  
  this->RecvLen = 0;
  
  int len = this->IO->Recv(this, tbuffer, sizeof(tbuffer) - 1);
  if (len <= 0)
    return false;
  
  tbuffer[len] = 0;
  this->RecvLen = len;
  
  Flux::string sbuffer = this->extrabuf;
  sbuffer += tbuffer;
  this->extrabuf.clear();
  size_t lastnewline = sbuffer.rfind('\n');
  if (lastnewline == Flux::string::npos)
  {
    this->extrabuf = sbuffer;
    return true;
  }
  if (lastnewline < sbuffer.length() - 1)
  {
    this->extrabuf = sbuffer.substr(lastnewline);
    this->extrabuf.trim();
    sbuffer = sbuffer.substr(0, lastnewline);
  }
  
  sepstream stream(sbuffer, '\n');
  
  Flux::string tbuf;
  while (stream.GetToken(tbuf))
  {
    tbuf.trim();
    if (!tbuf.empty() && !Read(tbuf))
      return false;
  }
  
  return true;
}

bool BufferedSocket::ProcessWrite()
{
  int count = this->IO->Send(this, this->WriteBuffer);
  if (count <= -1)
    return false;
  this->WriteBuffer = this->WriteBuffer.substr(count);
  if (this->WriteBuffer.empty())
    SocketEngine::ClearWritable(this);
  
  return true;
}

bool BufferedSocket::Read(const Flux::string &buf)
{
  return false;
}

void BufferedSocket::Write(const char *message, ...)
{
  va_list vi;
  char tbuffer[BUFSIZE];
  
  if (!message)
    return;
  
  va_start(vi, message);
  vsnprintf(tbuffer, sizeof(tbuffer), message, vi);
  va_end(vi);
  
  Flux::string sbuf = tbuffer;
  Write(sbuf);
}

void BufferedSocket::Write(const Flux::string &message)
{
  this->WriteBuffer += message + "\r\n";
  SocketEngine::MarkWritable(this);
}

int BufferedSocket::ReadBufferLen() const
{
  return RecvLen;
}

int BufferedSocket::WriteBufferLen() const
{
  return this->WriteBuffer.length();
}


BinarySocket::DataBlock::DataBlock(const char *b, size_t l)
{
  this->buf = new char[l];
  memcpy(this->buf, b, l);
  this->len = l;
}

BinarySocket::DataBlock::~DataBlock()
{
  delete [] this->buf;
}

BinarySocket::BinarySocket(){}

BinarySocket::~BinarySocket(){}

bool BinarySocket::ProcessRead()
{
  char tbuffer[NET_BUFSIZE];
  
  int len = this->IO->Recv(this, tbuffer, sizeof(tbuffer));
  if (len <= 0)
    return false;
  
  return this->Read(tbuffer, len);
}

bool BinarySocket::ProcessWrite()
{
  if (this->WriteBuffer.empty())
  {
    SocketEngine::ClearWritable(this);
    return true;
  }
  
  DataBlock *d = this->WriteBuffer.front();
  
  int len = this->IO->Send(this, d->buf, d->len);
  if (len <= -1)
    return false;
  else if (static_cast<size_t>(len) == d->len)
  {
    delete d;
    this->WriteBuffer.pop_front();
  }
  else
  {
    d->buf += len;
    d->len -= len;
  }
  
  if (this->WriteBuffer.empty())
    SocketEngine::ClearWritable(this);
  
  return true;
}

void BinarySocket::Write(const char *buffer, size_t l)
{
  this->WriteBuffer.push_back(new DataBlock(buffer, l));
  SocketEngine::MarkWritable(this);
}

bool BinarySocket::Read(const char *buffer, size_t l)
{
  return true;
}

ConnectionSocket::ConnectionSocket() : Socket(){}

void ConnectionSocket::Connect(const Flux::string &TargetHost, int Port)
{
  this->IO->Connect(this, TargetHost, Port);
}

bool ConnectionSocket::Process()
{
  try
  {
    if (this->IsConnected())
      return true;
    else if (this->IsConnecting())
      //this->SetFlag(this->IO->FinishConnect(this));
      this->SetStatus(this->IO->FinishConnect(this));
    else
      this->SetDead(true);
  }
  catch (const SocketException &ex)
  {
    Log() << ex.GetReason();
  }
  return false;
}

void ConnectionSocket::ProcessError()
{
  int optval = 0;
  socklen_t optlen = sizeof(optval);
  getsockopt(this->GetFD(), SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&optval), &optlen);
  errno = optval;
  this->OnError(optval ? strerror(errno) : "");
}

void ConnectionSocket::OnConnect()
{
}

void ConnectionSocket::OnError(const Flux::string &)
{
}

ClientSocket::ClientSocket(ListenSocket *ls, const sockaddrs &addr) : Socket(), LS(ls), clientaddr(addr)
{
}

bool ClientSocket::Process()
{
  try
  {
    if (this->IsAccepted())
      return true;
    else if (this->IsAccepting())
//       this->SetFlag(this->IO->FinishAccept(this));
	this->SetStatus(this->IO->FinishAccept(this));
    else
      this->SetDead(true);
  }
  catch (const SocketException &ex)
  {
    Log() << ex.GetReason();
  }
  return false;
}

void ClientSocket::ProcessError()
{
  int optval = 0;
  socklen_t optlen = sizeof(optval);
  getsockopt(this->GetFD(), SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&optval), &optlen);
  errno = optval;
  this->OnError(optval ? strerror(errno) : "");
}

void ClientSocket::OnAccept()
{
}

void ClientSocket::OnError(const Flux::string &error)
{
}

/*********************************************************************************/

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
  
  for (std::map<int, Socket *>::const_iterator it = Sockets.begin(), it_end = Sockets.end(); it != it_end;)
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
    for (std::map<int, Socket *>::const_iterator it = Sockets.begin(), it_end = Sockets.end(); it != it_end && processed != sresult;)
    {
      Socket *s = it->second;
      ++it;
      
      bool has_read = FD_ISSET(s->GetFD(), &rfdset), has_write = FD_ISSET(s->GetFD(), &wfdset), has_error = FD_ISSET(s->GetFD(), &efdset);
      if (has_read || has_write || has_error)
	++processed;
      Log(LOG_TERMINAL) << "Processed " << s->GetFD();
      if (has_error)
      {
	s->ProcessError();
	s->SetDead(true);
	delete s;
	continue;
      }
      
      if (!s->Process())
	continue;
      
      if (has_read && !s->ProcessRead())
	s->SetDead(true);
      
      if (has_write && !s->ProcessWrite())
	s->SetDead(true);
      
      if (s->IsDead()){
	Log(LOG_TERMINAL) << "Socket " << s->GetFD() << " died!";
	delete s;
      }
    }
  }
}

/*********************************************************************************/
// bool throwex;
// Flux::string throwmsg;
// fd_set ReadFD/*, WriteFD, ExceptFD*/;
// 
// /* FIXME: please god, when will the hurting stop? This class is so
//    f*cking broken it's not even funny */
// SocketIO::SocketIO(const Flux::string &cserver, const Flux::string &cport) : sockn(-1){
//   SET_SEGV_LOCATION();
//   this->server = cserver;
//   this->port = cport;
//   throwex = false;
//   
//   memset(&hints, 0, sizeof(hints));
//   hints.ai_family = AF_INET;
//   hints.ai_socktype = SOCK_STREAM;
//   /****************************/ 
// }
// int SocketIO::GetFD() const { return sockn; }
// void SocketIO::ThrowException(const Flux::string &msg) { throwex = true; throwmsg = msg; }
// bool SocketIO::SetNonBlocking()
// {
//  int flags = fcntl(this->GetFD(), F_GETFL, 0);
//  return !fcntl(this->GetFD(), F_SETFL, flags | O_NONBLOCK);
// }
// bool SocketIO::SetBlocking()
// {
//   int flags = fcntl(this->GetFD(), F_GETFL, 0);
//   return !fcntl(this->GetFD(), F_SETFL, flags & ~O_NONBLOCK);
// }
// SocketIO::~SocketIO(){
//   SET_SEGV_LOCATION();
//  if(is_valid()) 
//    close(sockn);
//  FD_CLR(this->GetFD(), &ReadFD);
//  //FD_CLR(this->GetFD(), &WriteFD);
// }
// void SocketIO::get_address()
// {
//   SET_SEGV_LOCATION();
//   int rv = 1;
//   rv = getaddrinfo(this->server.c_str(), this->port.c_str(), &hints, &servinfo);
//   if (rv != 0)
//   {
//     throw SocketException(fsprintf("Could not resolve server: %s:%i %s",this->server.c_str(), 
// 				   (int)this->port, gai_strerror(rv)).c_str());
//   }
// }
// 
// void *get_in_addr(struct sockaddr *sa)
// {
//   SET_SEGV_LOCATION();
//     if (sa->sa_family == AF_INET) {
//         return &(((struct sockaddr_in*)sa)->sin_addr);
//     }
//     return &(((struct sockaddr_in6*)sa)->sin6_addr);
// }
// 
// bool SocketIO::Connect()
// {
//   SET_SEGV_LOCATION();
//   struct addrinfo *p;
//   int connected = 0;
//   char s[INET6_ADDRSTRLEN];
//   
//   this->get_address(); // catch any resolution issues before we even attempt to connect..
//   
//   for(p = servinfo; p != NULL; p = p->ai_next)
//   {
//     sockn = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
//     if (this->GetFD() < 0) 
//       continue;
//     connected = connect(this->GetFD(), p->ai_addr, p->ai_addrlen);
//     if (connected == -1)
//     {
//       close(sockn);
//       continue;
//     }
//     break;
//   }
//   
//   if (connected == -1)
//     throw SocketException("Connection Failed.");
//     
//   freeaddrinfo(servinfo); //Clear up used memory we dont need anymore
//   
//   inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, INET6_ADDRSTRLEN+1);
//   this->SetNonBlocking();
//   FD_SET(this->GetFD(), &ReadFD);
//   Log(LOG_DEBUG) << "Connected to" << this->server << ":" << this->port;
//   return true;
// }
// int SocketIO::Process()
// {
//   timeval timeout;
//   timeout.tv_sec = Config->SockWait;
//   timeout.tv_usec = 0; //this timeout keeps the bot from being a CPU hog for no reason :)
//   fd_set read = ReadFD/*, write = WriteFD, except = ExceptFD*/;
//   FD_ZERO(&read);
//   FD_SET(this->GetFD(), &read);
//   int sres = select(this->GetFD() + 1, &read, NULL, NULL, &timeout);
//   if(sres == -1 && errno != EINTR){
//     Log(LOG_DEBUG) << "Select() error: " << strerror(errno);
//     return errno;
//   }
//   if(throwex) //throw a socket exception if we want to. mainly used for ping timeouts.
//     throw SocketException(throwmsg);
//   if(FD_ISSET(this->GetFD(), &read) && sres)
//   {
//   if(this->recv() == -1 && !quitting)
//       {
// 	Log(LOG_RAWIO) << "Socket Error: " << strerror(errno);
// 	return errno;
//       }else
//       {
// 	return this->recv();
//       }
//   }
//   return sres;
// }
// int SocketIO::recv() const
// {
//   char tbuf[BUFSIZE + 1] = "";
//   memset(tbuf, 0, BUFSIZE + 1);
//   size_t i = read(this->GetFD(), tbuf, BUFSIZE);
//   if(i <= 0)
//     return i;
//   sepstream sep(tbuf, '\n');
//   Flux::string buf;
//   while(sep.GetToken(buf))
//   {
//     buf.trim();
//     LastBuf = buf;
//     this->Read(buf);
//   }
//   return i;
// }
// int SocketIO::send(const Flux::string buf) const
// {
//  LastBuf = buf;
//  Log(LOG_RAWIO) << "Sent: " << buf << " | Size: " << buf.size();
//  if(!protocoldebug)
//    Log(LOG_DEBUG) << buf;
//  int i = write(this->GetFD(), buf.c_str(), buf.size());
//  return i;
// }




/** \fn void send_cmd(const char *fmt, ...)
 * \brief Sends something directly out the socket after being processed by vsnprintf
 * \param char* a string of what to send to the server including printf style format
 * \param va_list all the variables to be replaced with the printf style variables
 */
void send_cmd(const char *fmt, ...)
{
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  if(Fluxsocket){
    Log(LOG_DEBUG) << buffer;
    Fluxsocket->Write(buffer);
  }else
    Log(LOG_DEBUG) << "Attempted to send \"" << buffer << "\"";
  va_end(args);
}