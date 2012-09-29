/* Arbitrary Navn Tool -- CTCP handling module
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#include "modules.h"

/*
 * This module is used to accept JSON-RPC based on the following link
 * http://code.google.com/p/support/wiki/PostCommitWebHooks
 *
 * this allows most websites to interface with CIA.vc without doing an
 * extreme change in their code to submit a XML-RPC request.
 */

// Module globals
class JSONListenSocket;
class JSONClient;
class JSONRPCModule;
class RetryStart;

std::vector<JSONListenSocket*> listen_sockets;
static const Flux::string systemver = value_cast<Flux::string>(VERSION_FULL);

// Listen socket used to see if there are new clients to accept from.
class JSONListenSocket : public ListenSocket
{
public:
    JSONListenSocket(const Flux::string &bindip, int port, bool ipv6) : ListenSocket(bindip, port, ipv6)
    {
	Log(LOG_RAWIO) << "[JSON-RPC] New Listen socket created " << bindip << ':' << port << (ipv6?" (IPv6)":" (IPv4)");
	listen_sockets.push_back(this);
    }

    ~JSONListenSocket()
    {
	auto it = std::find(listen_sockets.begin(), listen_sockets.end(), this);
	if(it != listen_sockets.end())
	    listen_sockets.erase(it);

	Log(LOG_DEBUG) << "[JSON-RPC] Deleting listen socket for " << this->bindaddr.addr() << ':' <<
	this->bindaddr.port() << (this->IsIPv6()?" (IPv6)":" (IPv4)");
    }

    ClientSocket *OnAccept(int fd, const sockaddrs &addr);
};

// Client socket class used to accept data from the JSON client
class JSONClient : public ClientSocket, public BufferedSocket, public Timer
{
public:
    JSONClient(JSONListenSocket *ls, int fd, const sockaddrs &addr) : Socket(fd, ls->IsIPv6()), ClientSocket(reinterpret_cast<ListenSocket*>(ls), addr), BufferedSocket(), Timer(Config->jsonrpctimeout)
    {
	Log(LOG_RAWIO) << "[JSON-RPC] Created and accepted client from " << this->clientaddr.addr();
    }

    ~JSONClient()
    {
	Log(LOG_DEBUG) << "[JSON-RPC] Finished accepting message from " << this->clientaddr.addr() << "!";
    }

    // Data from socket!
    bool Read(const Flux::string &m)
    {
	// TODO: For now just print to the terminal whatever we receive.
	Log(LOG_RAWIO) << m;
	//Log(LOG_RAWIO) << "\nFinished receiving JSON-RPC!";
	return true;
    }

    // very basic HTTP reply method.
    void HTTPReply(int code, const Flux::string &statustype, const Flux::string &contenttype, const Flux::string &message)
    {
	// Basic header
	this->Write("HTTP/1.1 %d %s", code, statustype.c_str());
	this->Write("CONNECTION: CLOSE");
	this->Write("DATE: "+do_strftime(time(NULL), true));
	this->Write("SERVER: ANT Commit System version " + systemver);

	// Some codes do not require content.
	if(!contenttype.empty() && !message.empty());
	{
	    this->Write("CONTENT-TYPE: %s", contenttype.c_str());
	    this->Write(printfify("CONTENT-LENGTH: %i", message.size()));
	}

	// That stupid fucking newline
	this->Write("");

	// Message content
	if(!message.empty())
	    this->Write(message);

	this->SetStatus(SF_DEAD, true);
    }

    // Timeout if someone idles on the server for too long without submitting data.
    void Tick(time_t)
    {
	Log(LOG_DEBUG) << "[JSON-RPC] Connection Timeout for " << this->clientaddr.addr() << ", closing connection.";
	this->SetDead(true);
    }
};


// Accept the client from the listen socket, create a client socket.
ClientSocket* JSONListenSocket::OnAccept(int fd, const sockaddrs &addr)
{
    ClientSocket *c = new JSONClient(this, fd, addr);
    return c;
}

// Stupid workaround for starting the socket engine after the modules are all initialized
// XXX: This is a FIXME! It's a NASTY HAXX!
class RetryStart : public Timer
{
public:
    RetryStart():Timer(10, time(NULL), false) {}
    void Tick(time_t)
    {
	try
	{
	    new JSONListenSocket(Config->jsonrpcbindip, Config->jsonrpcport, Config->jsonrpcipv6);
	}
	catch(const SocketException &ex)
	{
	    Log() << "[JSON-RPC] " << ex.GetReason();
	    new RetryStart();
	}
    }
};

// Module interface
class JSONRPCModule : public Module
{
public:
    JSONRPCModule(const Flux::string &Name) : Module(Name, MOD_NORMAL)
    {
	this->SetAuthor("Justasic");
	this->SetVersion(VERSION);

	// Try and make a socket.
	new RetryStart();
    }

    ~JSONRPCModule()
    {
	for(auto it : SocketEngine::Sockets)
	{
	    Socket *s = it.second;
	    ClientSocket *cs = dynamic_cast<ClientSocket*>(s);
	    if(cs)
	    {
		for(unsigned i=0; i < listen_sockets.size(); ++i)
		    if(cs->LS == listen_sockets[i])
		    {
			delete cs;
			break;
		    }
	    }
	}

	for(std::vector<JSONListenSocket*>::iterator it = listen_sockets.begin(), it_end = listen_sockets.end(); it != it_end; ++it)
	    delete *it;
    }

};

// Module hook to allow the module to be loaded by the core
MODULE_HOOK(JSONRPCModule)