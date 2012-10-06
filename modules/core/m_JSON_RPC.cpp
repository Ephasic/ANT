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

//TODO: Modify this!
static const Flux::string HTTPREQUEST =
"<center><h1>ANT Commit system version "+systemver+"</h1></center>\n"
"<center><h4>This is the address for JSON-RPC commits</h4>\n"
"<p>This will not provide JSON-RPC requests and ONLY uses POST to commit the data (same as most <a href=\"http://cia.vc/doc/clients/\">CIA.vc scripts</a>), if you are looking for the site, please see <a href=\"http://www.Azuru.net/\">Azuru.net</a> for the sites location or optionally connect to Azuru IRC for support:</p>\n"
"<a href=\"irc://irc.Azuru.net/Computers\"><FONT COLOR=\"red\">irc.Azuru.net</FONT>:<FONT COlOR=\"Blue\">6667</FONT></a></br>\n"
"Channel: <FONT COLOR=\"Green\">#Commits</FONT></br>\n"
"Channel: <FONT COLOR=\"Green\">#Computers</FONT></br></center>\n";

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
    bool in_header, is_httpreq, processpost, has_google_hmac;
    Flux::vector httpheader, httpcontent;
public:
    JSONClient(JSONListenSocket *ls, int fd, const sockaddrs &addr) : Socket(fd, ls->IsIPv6()), ClientSocket(reinterpret_cast<ListenSocket*>(ls), addr), BufferedSocket(), Timer(Config->jsonrpctimeout),
    in_header(true), is_httpreq(false), processpost(false), has_google_hmac(false)
    {
	Log(LOG_RAWIO) << "[JSON-RPC] Created and accepted client from " << this->clientaddr.addr();
    }

    // Data from socket!
    bool Read(const Flux::string &message)
    {
// 	Flux::string message = SanitizeXML(m);
	Log(LOG_TERMINAL) << "Message: \"" << message << "\"";
	// According to the HTTP protocol, content and header are deliminated by
	// a newline which the socket engine interprets as a blank/empty line.
	// In my opinion this is the worst deliminator in the world and whoever
	// thought it up should be tortured and burned on the stake.
	if(message.empty())
	{
	    in_header = false;

	    if(processpost)
	    {
// 		this->HTTPReply(202, "Accepted", true, "text/plain", "Message accepted into queue");
// 		this->ProcessWrite();
	    }

	    if(has_google_hmac)
	    {
		this->Write("Authenticated");
		return true;
	    }

	    if(is_httpreq)
	    {
// 		this->HTTPReply(200, "OK", "text/html", HTTPREQUEST);
// 		return false;
	    }
	}

	// the method line
	if(in_header && !message.search(':') && !message.empty())
	{
	    Flux::vector line = ParamitizeString(message, ' ');
	    if(line.size() < 3)
	    {
		// Oh noes! Someone sent bad data!
// 		this->HTTPReply(400, "Bad Request", "", "");
		Log(LOG_DEBUG) << "Invalid or malformed syntax!";
// 		return false;
	    }

	    if(line[0].equals_ci("GET"))
		is_httpreq = true;
	    else if(line[0].equals_ci("POST"))
	    {
		// Process XML data for Commit, nothing to do here!
		processpost = true;
		return true;
	    }
	    else
	    {
		// invalid request, return 405!
		this->HTTPReply(405, "Method Not Allowed", false, "", "");
		Log(LOG_DEBUG) << "Invalid method request: " << line[0];
// 		return false;
	    }
	}
	// Other header info
	else if(in_header)
	{
	    httpheader.push_back(message);

	    Flux::vector line = ParamitizeString(message, ':');

// 	    if(line.size() == 2 && line[0].equals_ci("Content-Type") && !line[1].search_ci("text/xml"))
// 	    {
// 		this->HTTPReply(415, "Unsupported Media Type", "", "");
// 		return false;
// 	    }

	    if(line.size() > 0 && line[0].equals_ci("Google-Code-Project-Hosting-Hook-HMAC"))
	    {
		// Skip google's stupid authentication crap since it's not needed
		has_google_hmac = true;
	    }

	    if(line.size() > 1 && line[0].equals_ci("User-Agent"))
		; // Eventually track user agents to see what everyone is using!
	}
	// CONTENT! :D
	else
	{
	    httpcontent.push_back(message);

	    // ###: This should go off content length to justify when the document ends.
	    if(/* ###: TODO: This.*/0)
	    {
		this->HandleMessage();
		// ###: Is this a proper reply?
// 		this->HTTPReply(200, "OK", "text/html", "Message accepted into queue.");
// 		return false;
	    }
	}

	this->ProcessWrite();
	return true;
    }

    // very basic HTTP reply method.
    void HTTPReply(int code, const Flux::string &statustype, bool keepalive, const Flux::string &contenttype, const Flux::string &message)
    {
	// Basic header
	this->Write("HTTP/1.1 %d %s", code, statustype.c_str());
	this->Write("CONNECTION: %s", keepalive ? "Keep-Alive" : "Close");
	this->Write("DATE: %s", do_strftime(time(NULL), true).c_str());
	this->Write("SERVER: ANT Commit System version %s", systemver.c_str());

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

	this->ProcessWrite();

// 	this->SetStatus(SF_DEAD, true);
    }

    // Timeout if someone idles on the server for too long without submitting data.
    void Tick(time_t)
    {
	Log(LOG_DEBUG) << "[JSON-RPC] Connection Timeout for " << this->clientaddr.addr() << ", closing connection.";
	this->HTTPReply(408, "Request Timeout", false, "", "");
	this->SetStatus(SF_DEAD, true);
    }

    void HandleMessage()
    {
	// TODO: This.
    }

    bool ProcessWrite()
    {
	if(!this->WriteBuffer.empty())
	    Log(LOG_TERMINAL) << "Process Write:\n" << this->WriteBuffer;
	return BufferedSocket::ProcessWrite() && ClientSocket::ProcessWrite();
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