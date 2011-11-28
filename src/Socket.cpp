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

SocketEngineBase *SocketEngine = NULL;
int32 TotalRead = 0;
int32 TotalWritten = 0;

SocketIO normalSocketIO;

/** Trims all the \r and \ns from the begining and end of a string
 * @param buffer The buffer to trim
 */
static void TrimBuf(std::string &buffer)
{
  while (!buffer.empty() && (buffer[0] == '\r' || buffer[0] == '\n'))
    buffer.erase(buffer.begin());
  while (!buffer.empty() && (buffer[buffer.length() - 1] == '\r' || buffer[buffer.length() - 1] == '\n'))
    buffer.erase(buffer.length() - 1);
}

/** Construct the object, sets everything to 0
 */
sockaddrs::sockaddrs() { this->clear(); }

/** Memset the object to 0
 */
void sockaddrs::clear() { memset(this, 0, sizeof(*this)); }

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
      if (inet_pton(type, address.c_str(), &sa4.sin_addr) < 1)
	throw SocketException(Flux::string("Invalid host: "+ strerror(errno)));
      sa4.sin_family = type;
      sa4.sin_port = htons(pport);
      return;
    case AF_INET6:
      if (inet_pton(type, address.c_str(), &sa6.sin6_addr) < 1)
	throw SocketException(Flux::string("Invalid host: "+ strerror(errno)));
      sa6.sin6_family = type;
      sa6.sin6_port = htons(pport);
      return;
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
      sa4.sin_addr = *reinterpret_cast<const in_addr*>(src);
      sa4.sin_family = type;
      return;
    case AF_INET6:
      sa6.sin6_addr = *reinterpret_cast<const in6_addr*>(src);
      sa6.sin6_family = type;
      return;
    default:
      break;
  }
  throw CoreException("Invalid socket type");
}

/** Default constructor
 */
SocketEngineBase::SocketEngineBase()
{
  #ifdef _WIN32
  if (WSAStartup(MAKEWORD(2, 0), &wsa))
    throw FatalException("Failed to initialize WinSock library");
  #endif
}

/** Default destructor
 */
SocketEngineBase::~SocketEngineBase()
{
  for (auto it : Sockets)
    delete it->second;
  this->Sockets.clear();
  #ifdef _WIN32
  WSACleanup();
  #endif
}

/** Receive something from the buffer
 * @param s The socket
 * @param buf The buf to read to
 * @param sz How much to read
 * @return Number of bytes received
 */
int SocketIO::Recv(Socket *s, char *buf, size_t sz) const
{
  size_t i = recv(s->GetFD(), buf, sz, 0);
  TotalRead += i;
  return i;
}

/** Write something to the socket
 * @param s The socket
 * @param buf What to write
 * @return Number of bytes written
 */
int SocketIO::Send(Socket *s, const Flux::string &buf) const
{
  size_t i = send(s->GetFD(), buf.c_str(), buf.length(), 0);
  TotalWritten += i;
  return i;
}

/** Accept a connection from a socket
 * @param s The socket
 */
void SocketIO::Accept(ListenSocket *s)
{
  sockaddrs conaddr;
  
  socklen_t size = sizeof(conaddr);
  int newsock = accept(s->GetFD(), &conaddr.sa, &size);
  
  #ifndef INVALID_SOCKET
  # define INVALID_SOCKET 0
  #endif
  
  if (newsock > 0 && newsock != INVALID_SOCKET)
    s->OnAccept(newsock, conaddr);
  else
    throw SocketException("Unable to accept connection: " + strerror(errno));
}

/** Connect the socket
 * @param s THe socket
 * @param target IP to connect to
 * @param port to connect to
 * @param bindip IP to bind to, if any
 */
void SocketIO::Connect(ConnectionSocket *s, const Flux::string &target, int port, const Flux::string &bindip)
{
  s->bindaddr.clear();
  s->conaddr.clear();
  
  if (!bindip.empty())
  {
    s->bindaddr.pton(s->IsIPv6() ? AF_INET6 : AF_INET, bindip, 0);
    if (bind(s->GetFD(), &s->bindaddr.sa, s->bindaddr.size()) == -1)
      throw SocketException(Flux::string("Unable to bind to address: "+strerror(errno)));
  }
  
  s->conaddr.pton(s->IsIPv6() ? AF_INET6 : AF_INET, target, port);
  if (connect(s->GetFD(), &s->conaddr.sa, s->conaddr.size()) == -1 && errno != EINPROGRESS)
    throw SocketException(Flux::string("Error connecting to server: "+strerror(errno)));
}

/** Empty constructor, used for things such as the pipe socket
 */
Socket::Socket()
{
  this->Type = SOCKTYPE_BASE;
  this->IO = &normalSocketIO;
}

/** Constructor
 * @param sock The socket
 * @param ipv6 IPv6?
 * @param type The socket type, defaults to SOCK_STREAM
 */
Socket::Socket(int sock, bool ipv6, int type)
{
  this->Type = SOCKTYPE_BASE;
  this->IO = &normalSocketIO;
  this->IPv6 = ipv6;
  if (sock == 0)
    this->Sock = socket(this->IPv6 ? AF_INET6 : AF_INET, type, 0);
  else
    this->Sock = sock;
  SocketEngine->AddSocket(this);
}

/** Default destructor
 */
Socket::~Socket()
{
  if (SocketEngine)
    SocketEngine->DelSocket(this);
  Close(this->Sock);
  this->IO->Destroy();
}

/** Get the socket FD for this socket
 * @return the fd
 */
int Socket::GetFD() const { return Sock; }
/** Check if this socket is IPv6
 * @return true or false
 */
bool Socket::IsIPv6() const { return IPv6; }
/** Mark a socket as blockig
 * @return true if the socket is now blocking
 */
bool Socket::SetBlocking()
{
  #ifdef _WIN32
  unsigned long opt = 0;
  return !ioctlsocket(this->GetFD(), FIONBIO, &opt);
  #else
  int flags = fcntl(this->GetFD(), F_GETFL, 0);
  return !fcntl(this->GetFD(), F_SETFL, flags & ~O_NONBLOCK);
  #endif
}

/** Mark a socket as non-blocking
 * @return true if the socket is now non-blocking
 */
bool Socket::SetNonBlocking()
{
  #ifdef _WIN32
  unsigned long opt = 1;
  return !ioctlsocket(this->GetFD(), FIONBIO, &opt);
  #else
  int flags = fcntl(this->GetFD(), F_GETFL, 0);
  return !fcntl(this->GetFD(), F_SETFL, flags | O_NONBLOCK);
  #endif
}

/** Called when there is something to be received for this socket
 * @return true on success, false to drop this socket
 */
bool Socket::ProcessRead(){ return true; }
/** Called when the socket is ready to be written to
 * @return true on success, false to drop this socket
 */
bool Socket::ProcessWrite(){ return true; }
/** Called when there is an error for this socket
 * @return true on success, false to drop this socket
 */
void Socket::ProcessError(){}
/** Constructor for pipe socket
 */
BufferedSocket::BufferedSocket() : Socket()
{
  this->Type = SOCKTYPE_BUFFERED;
}

/** Constructor
 * @param fd FD to use
 * @param ipv6 true for ipv6
 * @param type socket type, defaults to SOCK_STREAM
 */
BufferedSocket::BufferedSocket(int fd, bool ipv6, int type) : Socket(fd, ipv6, type)
{
  this->Type = SOCKTYPE_BUFFERED;
}

/** Default destructor
 */
BufferedSocket::~BufferedSocket()
{
}

/** Called when there is something to be received for this socket
 * @return true on success, false to drop this socket
 */
bool BufferedSocket::ProcessRead()
{
  char tbuffer[NET_BUFSIZE] = "";
  
  RecvLen = this->IO->Recv(this, tbuffer, sizeof(tbuffer) - 1);
  if (RecvLen <= 0)
    return false;
  
  std::string sbuffer = extrabuf;
  sbuffer.append(tbuffer);
  extrabuf.clear();
  size_t lastnewline = sbuffer.rfind('\n');
  if (lastnewline == std::string::npos)
  {
    extrabuf = sbuffer;
    return true;
  }
  if (lastnewline < sbuffer.size() - 1)
  {
    extrabuf = sbuffer.substr(lastnewline);
    TrimBuf(extrabuf);
    sbuffer = sbuffer.substr(0, lastnewline);
  }
  
  sepstream stream(sbuffer, '\n');
  
  Flux::string tbuf;
  while (stream.GetToken(tbuf))
  {
    std::string tmp_tbuf = tbuf.str();
    TrimBuf(tmp_tbuf);
    tbuf = tmp_tbuf;
    
    if (!tbuf.empty())
      if (!Read(tbuf))
	return false;
  }
  
  return true;
}

/** Called when the socket is ready to be written to
 * @return true on success, false to drop this socket
 */
bool BufferedSocket::ProcessWrite()
{
  if (this->WriteBuffer.empty())
    return true;
  int count = this->IO->Send(this, this->WriteBuffer);
  if (count == -1)
    return false;
  this->WriteBuffer = this->WriteBuffer.substr(count);
  if (this->WriteBuffer.empty())
    SocketEngine->ClearWritable(this);
  
  return true;
}

/** Called with a line received from the socket
 * @param buf The line
 * @return true to continue reading, false to drop the socket
 */
bool BufferedSocket::Read(const Flux::string &buf)
{
  return false;
}

/** Write to the socket
 * @param message The message
 */
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

/** Write to the socket
 * @param message The message
 */
void BufferedSocket::Write(const Flux::string &message)
{
  WriteBuffer.append(message.str() + "\r\n");
  SocketEngine->MarkWritable(this);
}

/** Get the length of the read buffer
 * @return The length of the read buffer
 */
size_t BufferedSocket::ReadBufferLen() const
{
  return RecvLen;
}

/** Get the length of the write buffer
 * @return The length of the write buffer
 */
size_t BufferedSocket::WriteBufferLen() const
{
  return WriteBuffer.length();
}

/** Constructor
 * @param bindip The IP to bind to
 * @param port The port to listen on
 * @param ipv6 true for ipv6
 */
ListenSocket::ListenSocket(const Flux::string &bindip, int port, bool ipv6) : Socket(0, ipv6)
{
  this->Type = SOCKTYPE_LISTEN;
  this->SetNonBlocking();
  
  this->listenaddrs.pton(IPv6 ? AF_INET6 : AF_INET, bindip, port);
  
  if (bind(Sock, &this->listenaddrs.sa, this->listenaddrs.size()) == -1)
    throw SocketException(Flux::string("Unable to bind to address: "+ strerror(errno)));
  
  if (listen(Sock, 5) == -1)
    throw SocketException(Flux::string("Unable to listen: "+ strerror(errno)));
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

/** Called when a connection is accepted
 * @param fd The FD for the new connection
 * @param addr The sockaddr for where the connection came from
 * @return The new socket
 */
ClientSocket *ListenSocket::OnAccept(int fd, const sockaddrs &addr)
{
  return new ClientSocket(this, fd, addr);
}

/** Constructor
 * @param ipv6 true to use IPv6
 * @param type The socket type, defaults to SOCK_STREAM
 */
ConnectionSocket::ConnectionSocket(bool ipv6, int type) : BufferedSocket(0, ipv6, type)
{
  this->Type = SOCKTYPE_CONNECTION;
}

/** Connect the socket
 * @param TargetHost The target host to connect to
 * @param Port The target port to connect to
 * @param BindHost The host to bind to for connecting
 */
void ConnectionSocket::Connect(const Flux::string &TargetHost, int Port, const Flux::string &BindHost)
{
  try
  {
    this->IO->Connect(this, TargetHost, Port, BindHost);
  }
  catch (const SocketException &)
  {
    delete this;
    throw;
  }
}

/** Constructor
 * @param ls Listen socket this connection is from
 * @param fd New FD for this socket
 * @param addr Address the connection came from
 */
ClientSocket::ClientSocket(ListenSocket *ls, int fd, const sockaddrs &addr) : BufferedSocket(fd, ls->IsIPv6()), LS(ls), clientaddr(addr)
{
  this->Type = SOCKTYPE_CLIENT;
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
  sock->send(buffer);
  va_end(args);
}