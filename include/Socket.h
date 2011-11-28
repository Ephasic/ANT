/* Socket.h */
/* All code is licensed under GNU General Public License GPL v3 (http://www.gnu.org/licenses/gpl.html) */
/**
 *\file  Socket.h 
 *\brief Socket header for Socket.cpp
 */
#ifndef SOCKET
#define SOCKET

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <istream>
#include <cstdarg>
#include <ostream>
#include <queue>
#include <arpa/inet.h>

#include "extern.h"
#include "log.h"
// const int MAXHOSTNAME = 200;
// const int MAXCONNECTIONS = 5;

/** A sockaddr union used to combine IPv4 and IPv6 sockaddrs
 */
union CoreExport sockaddrs
{
  sockaddr sa;
  sockaddr_in sa4;
  sockaddr_in6 sa6;
  
  /** Construct the object, sets everything to 0
   */
  sockaddrs();
  
  /** Memset the object to 0
   */
  void clear();
  
  /** Get the size of the sockaddr we represent
   * @return The size
   */
  size_t size() const;
  
  /** Get the port represented by this addr
   * @return The port, or -1 on fail
   */
  int port() const;
  
  /** Get the address represented by this addr
   * @return The address
   */
  Flux::string addr() const;
  
  /** Check if this sockaddr has data in it
   */
  bool operator()() const;
  
  /** Compares with sockaddr with another. Compares address type, port, and address
   * @return true if they are the same
   */
  bool operator==(const sockaddrs &other) const;
  /* The same as above but not */
  inline bool operator!=(const sockaddrs &other) const { return !(*this == other); }
  
  /** The equivalent of inet_pton
   * @param type AF_INET or AF_INET6
   * @param address The address to place in the sockaddr structures
   * @param pport An option port to include in the  sockaddr structures
   * @throws A socket exception if given invalid IPs
   */
  void pton(int type, const Flux::string &address, int pport = 0);
  
  /** The equivalent of inet_ntop
   * @param type AF_INET or AF_INET6
   * @param address The in_addr or in_addr6 structure
   * @throws A socket exception if given an invalid structure
   */
  void ntop(int type, const void *src);
};

enum SocketType
{
  SOCKTYPE_BASE,
  SOCKTYPE_BUFFERED,
  SOCKTYPE_CONNECTION,
  SOCKTYPE_CLIENT,
  SOCKTYPE_LISTEN
};

class Socket;
class ClientSocket;
class ListenSocket;
class ConnectionSocket;

class CoreExport SocketIO
{
public:
  /** Receive something from the buffer
   * @param s The socket
   * @param buf The buf to read to
   * @param sz How much to read
   * @return Number of bytes received
   */
  virtual int Recv(Socket*, char*, size_t sz) const;
  
  /** Write something to the socket
   * @param s The socket
   * @param buf What to write
   * @return Number of bytes written
   */
  virtual int Send(Socket*, const Flux::string &buf) const;
  
  /** Accept a connection from a socket
   * @param s The socket
   */
  virtual void Accept(ListenSocket*);
  
  /** Connect the socket
   * @param s THe socket
   * @param target IP to connect to
   * @param port to connect to
   * @param bindip IP to bind to, if any
   */
  virtual void Connect(ConnectionSocket*, const Flux::string &target, int port, const Flux::string &bindip = "");
  
  /** Called when the socket is destructing
   */
  virtual void Destroy() { }
};

class CoreExport Socket
{
protected:
  int Sock;
  bool IPv6;
public:
  SocketIO *IO;
  Socket();
  Socket(int sock, bool ipv6 = false; int type = SOCK_STREAM);
  virtual ~Socket();
  int GetFD() const;
  bool IsIPv6() const;
  bool SetBlocking();
  bool SetNonBlocking();
  void Bind(const Flux::string &ip, int port = 0);
  virtual bool Process();
  virtual bool ProcessRead();
  virtual bool ProcessWrite();
  virtual bool ProcessError();
};
class CoreExport BinarySocket : public virtual Socket
{
  struct DataBlock
  {
    char *buf;
    size_t len;
    DataBlock(const char*, size_t);
    ~DataBlock();
  }
  std::deque<DataBlock*> WriteBuffer;
public:
  BinarySocket();
  virtual ~BinarySocket();
  bool ProcessWrite();
  bool ProcessRead();
  void Write(const char*, size_t);
  virtual bool Read(const char*, size_t);
};

class CoreExport BufferedSocket : public Socket
{
protected:
  /* Things to be written to the socket */
  base_string WriteBuffer;
  /* Part of a message sent from the server, but not totally received */
  base_string extrabuf;
  /* How much data was received from this socket */
  size_t RecvLen;
  
public:
  /** Blank constructor
   */
  BufferedSocket();
  
  /** Constructor
   * @param fd FD to use
   * @param ipv6 true for ipv6
   * @param type socket type, defaults to SOCK_STREAM
   */
  BufferedSocket(int fd, bool ipv6, int type = SOCK_STREAM);
  
  /** Default destructor
   */
  virtual ~BufferedSocket();
  
  /** Called when there is something to be received for this socket
   * @return true on success, false to drop this socket
   */
  bool ProcessRead();
  
  /** Called when the socket is ready to be written to
   * @return true on success, false to drop this socket
   */
  bool ProcessWrite();
  
  /** Called with a line received from the socket
   * @param buf The line
   * @return true to continue reading, false to drop the socket
   */
  virtual bool Read(const Flux::string &buf);
  
  /** Write to the socket
   * @param message The message
   */
  void Write(const char *message, ...);
  void Write(const Flux::string &message);
  
  /** Get the length of the read buffer
   * @return The length of the read buffer
   */
  size_t ReadBufferLen() const;
  
  /** Get the length of the write buffer
   * @return The length of the write buffer
   */
  size_t WriteBufferLen() const;
};

class CoreExport ListenSocket : public Socket
{
protected:
  /* Sockaddrs for bindip/port */
  sockaddrs listenaddrs;
  
public:
  /** Constructor
   * @param bindip The IP to bind to
   * @param port The port to listen on
   * @param ipv6 true for ipv6
   */
  ListenSocket(const Flux::string &bindip, int port, bool ipv6);
  
  /** Destructor
   */
  virtual ~ListenSocket();
  
  /** Process what has come in from the connection
   * @return false to destory this socket
   */
  bool ProcessRead();
  
  /** Called when a connection is accepted
   * @param fd The FD for the new connection
   * @param addr The sockaddr for where the connection came from
   * @return The new socket
   */
  virtual ClientSocket *OnAccept(int fd, const sockaddrs &addr);
};

class CoreExport ConnectionSocket : public BufferedSocket
{
public:
  /* Sockaddrs for bindip (if there is one) */
  sockaddrs bindaddr;
  /* Sockaddrs for connection ip/port */
  sockaddrs conaddr;
  
  /** Constructor
   * @param ipv6 true to use IPv6
   * @param type The socket type, defaults to SOCK_STREAM
   */
  ConnectionSocket(bool ipv6 = false, int type = SOCK_STREAM);
  
  /** Connect the socket
   * @param TargetHost The target host to connect to
   * @param Port The target port to connect to
   * @param BindHost The host to bind to for connecting
   */
  void Connect(const Flux::string &TargetHost, int Port, const Flux::string &BindHost = "");
};

class ClientSocket : public BufferedSocket
{
  /* Listen socket this connection came from */
  ListenSocket *LS;
  /* Clients address */
  sockaddrs clientaddr;
public:
  
  /** Constructor
   * @param ls Listen socket this connection is from
   * @param fd New FD for this socket
   * @param addr Address the connection came from
   */
  ClientSocket(ListenSocket *ls, int fd, const sockaddrs &addr);
};

class CoreExport SocketEngineBase
{
  static std::map<int, Socket*> Sockets;
  static void Init();
  static void ShutDown();
  static void AddSocket(Socket*);
  static void DelSocket(Socket*);
  static void MarkWritable(Socket*);
  static void ClearWritable(Socket*);
  static void Process();
};
// class CoreExport SocketIO2
// {
// private:
//   Flux::string server, port;
//   int sockn;
//   size_t recvlen;
//   struct addrinfo hints, *servinfo;
// public:
//   SocketIO(const Flux::string &server, const Flux::string &port);
//   ~SocketIO();
//   void get_address();
//   int GetFD() const;
//   bool SetNonBlocking();
//   bool SetBlocking();
//   int recv() const;
//   int send(const Flux::string buf) const;
//   bool Connect();
//   bool is_valid() const { return this->GetFD() != -1; }
//   int Process();
//   bool Read(const Flux::string&) const;
//   void ThrowException(const Flux::string&);
// };
#endif