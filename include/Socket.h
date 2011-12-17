/* Socket.h */
/* All code is licensed under GNU General Public License GPL v3 (http://www.gnu.org/licenses/gpl.html) */
/**
 *\file  Socket.h 
 *\brief Socket header for Socket.cpp
 */
#ifndef _SOCKET_H
#define _SOCKET_H

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
#include "SocketException.h"
#include "log.h"

#define NET_BUFSIZE 65535

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

class CoreExport cidr
{
  sockaddrs addr;
  Flux::string cidr_ip;
  unsigned char cidr_len;
public:
  cidr(const Flux::string &ip);
  cidr(const Flux::string &ip, unsigned char len);
  Flux::string mask() const;
  bool match(sockaddrs &other);
};

class SocketException : public CoreException
{
public:
  /** Default constructor for socket exceptions
   * @param message Error message
   */
  SocketException(const Flux::string &message) : CoreException(message) { }
  
  /** Default destructor
   * @throws Nothing
   */
  virtual ~SocketException() throw() { }
};

enum SocketFlag
{
  SF_DEAD,
  SF_WRITABLE,
  SF_CONNECTING,
  SF_CONNECTED,
  SF_ACCEPTING,
  SF_ACCEPTED
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
  virtual int Recv(Socket *s, char *buf, size_t sz);
  
  /** Write something to the socket
   * @param s The socket
   * @param buf The data to write
   * @param size The length of the data
   */
  virtual int Send(Socket *s, const char *buf, size_t sz);
  int Send(Socket *s, const Flux::string &buf);
  
  /** Accept a connection from a socket
   * @param s The socket
   * @return The new socket
   */
  virtual ClientSocket *Accept(ListenSocket *s);
  
  /** Finished accepting a connection from a socket
   * @param s The socket
   * @return SF_ACCEPTED if accepted, SF_ACCEPTING if still in process, SF_DEAD on error
   */
  virtual SocketFlag FinishAccept(ClientSocket *cs);
  
  /** Bind a socket
   * @param s The socket
   * @param ip The IP to bind to
   * @param port The optional port to bind to
   */
  virtual void Bind(Socket *s, const Flux::string &ip, int port = 0);
  
  /** Connect the socket
   * @param s The socket
   * @param target IP to connect to
   * @param port to connect to
   */
  virtual void Connect(ConnectionSocket *s, const Flux::string &target, int port);
  
  /** Called to potentially finish a pending connection
   * @param s The socket
   * @return SF_CONNECTED on success, SF_CONNECTING if still pending, and SF_DEAD on error.
   */
  virtual SocketFlag FinishConnect(ConnectionSocket *s);
  
  /** Called when the socket is destructing
   */
  virtual void Destroy() { }
};

class CoreExport Socket
{
protected:
  /* Socket FD */
  int Sock;
  /* Is this socket dead? */
  bool isdead;
  /* Is this an IPv6 socket? */
  bool IPv6;
  /* Other Booleans */
  bool isconnecting, isconnected, isaccepting, isaccepted, iswritable;
  
public:
  /* Sockaddrs for bind() (if it's bound) */
  sockaddrs bindaddr;
  
  /* I/O functions used for this socket */
  SocketIO *IO;
  
  /** Empty constructor, should not be called.
   */
  Socket();
  
  /** Default constructor
   * @param sock The socket to use, -1 if we need to create our own
   * @param ipv6 true if using ipv6
   * @param type The socket type, defaults to SOCK_STREAM
   */
  Socket(int sock, bool ipv6 = false, int type = SOCK_STREAM);
  
  /** Default destructor
   */
  virtual ~Socket();
  
  /** Get the socket FD for this socket
   * @return the fd
   */
  int GetFD() const;

  /** Set the socket as being dead
   * @param bool boolean if the socket is dead
   */
  void SetDead(bool);
  bool IsDead();

  void SetStatus(SocketFlag, bool);
  void SetConnecting(bool);
  bool IsConnecting();
  void SetConnected(bool);
  bool IsConnected();
  void SetAccepting(bool);
  bool IsAccepting();
  void SetAccepted(bool);
  bool IsAccepted();
  void SetWritable(bool);
  bool IsWritable();
  
  /** Check if this socket is IPv6
   * @return true or false
   */
  bool IsIPv6() const;
  
  /** Mark a socket as blockig
   * @return true if the socket is now blocking
   */
  bool SetBlocking();
  
  /** Mark a socket as non-blocking
   * @return true if the socket is now non-blocking
   */
  bool SetNonBlocking();
  
  /** Bind the socket to an ip and port
   * @param ip The ip
   * @param port The port
   */
  void Bind(const Flux::string &ip, int port = 0);
  
  /** Called when there either is a read or write event.
   * @return true to continue to call ProcessRead/ProcessWrite, false to not continue
   */
  virtual bool Process();
  
  /** Called when there is something to be received for this socket
   * @return true on success, false to drop this socket
   */
  virtual bool ProcessRead();
  
  /** Called when the socket is ready to be written to
   * @return true on success, false to drop this socket
   */
  virtual bool ProcessWrite();
  
  /** Called when there is an error for this socket
   * @return true on success, false to drop this socket
   */
  virtual void ProcessError();
};

class CoreExport BufferedSocket : public virtual Socket
{
protected:
  /* Things to be written to the socket */
  Flux::string WriteBuffer;
  /* Part of a message sent from the server, but not totally received */
  Flux::string extrabuf;
  /* How much data was received from this socket */
  int RecvLen;
  
public:
  /** Constructor
   */
  BufferedSocket();
  
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
  int ReadBufferLen() const;
  
  /** Get the length of the write buffer
   * @return The length of the write buffer
   */
  int WriteBufferLen() const;
};

class CoreExport BinarySocket : public virtual Socket
{
  struct DataBlock
  {
    char *buf;
    size_t len;
    
    DataBlock(const char *b, size_t l);
    ~DataBlock();
  };
  
  std::deque<DataBlock *> WriteBuffer;
  
public:
  /** Constructor
   */
  BinarySocket();
  
  /** Default destructor
   */
  virtual ~BinarySocket();
  
  /** Called when there is something to be received for this socket
   * @return true on success, false to drop this socket
   */
  bool ProcessRead();
  
  /** Called when the socket is ready to be written to
   * @return true on success, false to drop this socket
   */
  bool ProcessWrite();
  
  /** Write data to the socket
   * @param buffer The data to write
   * @param l The length of the data
   */
  void Write(const char *buffer, size_t l);
  
  /** Called with data from the socket
   * @param buffer The data
   * @param l The length of buffer
   * @return true to continue reading, false to drop the socket
   */
  virtual bool Read(const char *buffer, size_t l);
};

class CoreExport ListenSocket : public Socket
{
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
  virtual ClientSocket *OnAccept(int fd, const sockaddrs &addr) = 0;
};

class CoreExport ConnectionSocket : public virtual Socket
{
public:
  /* Sockaddrs for connection ip/port */
  sockaddrs conaddr;
  
  /** Constructor
   */
  ConnectionSocket();
  
  /** Connect the socket
   * @param TargetHost The target host to connect to
   * @param Port The target port to connect to
   */
  void Connect(const Flux::string &TargetHost, int Port);
  
  /** Called when there either is a read or write event.
   * Used to determine whether or not this socket is connected yet.
   * @return true to continue to call ProcessRead/ProcessWrite, false to not continue
   */
  bool Process();
  
  /** Called when there is an error for this socket
   * @return true on success, false to drop this socket
   */
  void ProcessError();
  
  /** Called on a successful connect
   */
  virtual void OnConnect();
  
  /** Called when a connection is not successful
   * @param error The error
   */
  virtual void OnError(const Flux::string &error);
};

class CoreExport ClientSocket : public virtual Socket
{
public:
  /* Listen socket this connection came from */
  ListenSocket *LS;
  /* Clients address */
  sockaddrs clientaddr;
  
  /** Constructor
   * @param ls Listen socket this connection is from
   * @param addr Address the connection came from
   */
  ClientSocket(ListenSocket *ls, const sockaddrs &addr);
  
  /** Called when there either is a read or write event.
   * Used to determine whether or not this socket is connected yet.
   * @return true to continue to call ProcessRead/ProcessWrite, false to not continue
   */
  bool Process();
  
  /** Called when there is an error for this socket
   * @return true on success, false to drop this socket
   */
  void ProcessError();
  
  /** Called when a client has been accepted() successfully.
   */
  virtual void OnAccept();
  
  /** Called when there was an error accepting the client
   */
  virtual void OnError(const Flux::string &error);
};

class CoreExport Pipe : public Socket
{
public:
  /** The FD of the write pipe (if this isn't evenfd)
   * this->Sock is the readfd
   */
  int WritePipe;
  
  /** Constructor
   */
  Pipe();
  
  /** Destructor
   */
  ~Pipe();
  
  /** Called when data is to be read
   */
  bool ProcessRead();
  
  /** Called when this pipe needs to be woken up
   */
  void Notify();
  
  /** Should be overloaded to do something useful
   */
  virtual void OnNotify();
};

/**********************************************************************/

class CoreExport SocketEngine
{
public:
  /* Map of sockets */
  static std::map<int, Socket *> Sockets;
  
  /** Called to initialize the socket engine
   */
  static void Init();
  
  /** Called to shutdown the socket engine
   */
  static void Shutdown();
  
  /** Add a socket to the internal list
   * @param s The socket
   */
  static void AddSocket(Socket *s);
  
  /** Delete a socket from the internal list
   * @param s The socket
   */
  static void DelSocket(Socket *s);
  
  /** Mark a socket as writeable
   * @param s The socket
   */
  static void MarkWritable(Socket *s);
  
  /** Unmark a socket as writeable
   * @param s The socket
   */
  static void ClearWritable(Socket *s);
  
  /** Read from sockets and do things
   */
  static void Process();
};

/* Base socket class for ALL network connections for the Network class */
/* This isnt in network.h because of some include recursion issues -_- */
class NetworkSocket : public ConnectionSocket, public BufferedSocket
{
public:
  NetworkSocket(Network*);
  ~NetworkSocket();
  Network *net;
  bool Read(const Flux::string&);
  bool ProcessWrite();
  void OnConnect();
  void OnError(const Flux::string&);
  bool Process();
};

#endif