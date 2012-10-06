/* Arbitrary Navn Tool -- XML-RPC Parser Module
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

/*
 * This is the module file for the XML_RPC commits, this will handle 98% of the commits the bot will process!
 */
#include "modules.h"
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_utils.hpp"
#include "rapidxml/rapidxml_iterators.hpp"
#include "rapidxml/rapidxml_print.hpp"

// Convert the XML data to something parsable.
Flux::string SanitizeXML(const Flux::string &str)
{
    static struct chars
    {
	Flux::string character;
	Flux::string replace;
	chars(const Flux::string &c, const Flux::string &r) : character(c), replace(r) { }
    }
    special[] = {
	chars("&amp;", "&"),
	chars("&quot;", "\""),
	chars("&lt;", "<"),
	chars("&gt;", ">"),
	chars("&#39", "'"),
	chars("&#xA", "\n"),
	chars("\r", ""),
	chars("", "")
    };

    Flux::string ret = str;
    for(int i = 0; special[i].character.empty() == false; ++i)
	ret = ret.replace_all_cs(special[i].character, special[i].replace);
    return ret;
}

// Simple web page in case a web browser decides to go to the link
static const Flux::string systemver = value_cast<Flux::string>(VERSION_FULL);
static const Flux::string HTTPREQUEST =
"<center><h1>ANT Commit system version "+systemver+"</h1></center>\n"
"<center><h4>This is the address for XML-RPC commits</h4>\n"
"<p>This will not provide XML-RPC requests and ONLY uses POST to commit the data (same as most <a href=\"http://cia.vc/doc/clients/\">CIA.vc scripts</a>), if you are looking for the site, please see <a href=\"http://www.Azuru.net/\">Azuru.net</a> for the sites location or optionally connect to Azuru IRC for support:</p>\n"
"<a href=\"irc://irc.Azuru.net/Computers\"><FONT COLOR=\"red\">irc.Azuru.net</FONT>:<FONT COlOR=\"Blue\">6667</FONT></a></br>\n"
"Channel: <FONT COLOR=\"Green\">#Commits</FONT></br>\n"
"Channel: <FONT COLOR=\"Green\">#Computers</FONT></br></center>\n";

class xmlrpcclient;
class xmlrpclistensocket;
class DNSResolver;
class xmlrpcmod;
static Module *me;
std::vector<xmlrpclistensocket*> listen_sockets;

/*****************************************************************/
/*********************** Listen Socket ***************************/
/*****************************************************************/
class xmlrpclistensocket : public ListenSocket
{
public:
    xmlrpclistensocket(const Flux::string &bindip, int port, bool ipv6) : ListenSocket(bindip, port, ipv6)
    {
	Log(LOG_RAWIO) << "[XML-RPC] New Listen socket created " << bindip << ':' << port << (ipv6?" (IPv6)":" (IPv4)");
	listen_sockets.push_back(this);
    }

    ~xmlrpclistensocket()
    {
	auto it = std::find(listen_sockets.begin(), listen_sockets.end(), this);
	if(it != listen_sockets.end())
	    listen_sockets.erase(it);

	Log(LOG_DEBUG) << "[XML-RPC] Deleting listen socket for " << this->bindaddr.addr() << ':' <<
	this->bindaddr.port() << (this->IsIPv6()?" (IPv6)":" (IPv4)");
    }

    ClientSocket *OnAccept(int fd, const sockaddrs &addr);
};

class DNSResolver : public DNSRequest
{
    Flux::string ip;
public:
    DNSResolver(const Flux::string &addr) : DNSRequest(addr, DNS_QUERY_PTR, true, me), ip(addr) { }

    void OnLookupComplete(const DNSQuery *record)
    {
// 	const ResourceRecord &ans_record = record->answers[0];
	Log(LOG_RAWIO) << "[XML-RPC] User connecting from " << record->answers[0].rdata << " (" << ip << ")";
    }
};



/*****************************************************************/
/*********************** Client Socket ***************************/
/*****************************************************************/

class xmlrpcclient : public ClientSocket, public BufferedSocket, public Timer
{
    Flux::vector httpheader, httpcontent;
    bool in_header, is_httpreq;
    size_t contentbytes;
public:
    xmlrpcclient(xmlrpclistensocket *ls, int fd, const sockaddrs &addr) : Socket(fd, ls->IsIPv6()),
    ClientSocket(reinterpret_cast<ListenSocket*>(ls), addr), BufferedSocket(), Timer(Config->xmlrpctimeout),
    in_header(true), is_httpreq(false), contentbytes(0) { }

    bool Read(const Flux::string &m)
    {
	Flux::string message = SanitizeXML(m);
	contentbytes += message.size();
        //Log(LOG_TERMINAL) << "Message: \"" << message << "\"";

	// According to the HTTP protocol, content and header are deliminated by
	// a newline which the socket engine interprets as a blank/empty line.
	// In my opinion this is the worst deliminator in the world and whoever
	// thought it up should be tortured and burned on the stake.
	if(message.empty())
	{
	    in_header = false;

	    if(is_httpreq)
	    {
		this->HTTPReply(200, "OK", "text/html", HTTPREQUEST);
		return false;
	    }

	    return true;
	}

	// the method line
	if(in_header && !message.search(':') && !message.empty())
	{
	    Flux::vector line = ParamitizeString(message, ' ');
	    if(line.size() < 3)
	    {
		// Oh noes! Someone sent bad data!
		this->HTTPReply(400, "Bad Request", "", "");
		Log(LOG_DEBUG) << "Invalid or malformed syntax!";
		return false;
	    }

	    if(line[0].equals_ci("GET"))
		is_httpreq = true;
	    else if(line[0].equals_ci("POST"))
	    {
		// Process XML data for Commit, nothing to do here!
	    }
	    else
	    {
		// invalid request, return 405!
		this->HTTPReply(405, "Method Not Allowed", "", "");
		Log(LOG_DEBUG) << "Invalid method request: " << line[0];
		return false;
	    }
	}
	// Other header info
	else if(in_header)
	{
	    httpheader.push_back(message);

	    Flux::vector line = ParamitizeString(message, ':');

	    if(line.size() == 2 && line[0].equals_ci("Content-Type") && !line[1].search_ci("text/xml"))
	    {
		this->HTTPReply(415, "Unsupported Media Type", "", "");
		return false;
	    }

	    if(line.size() > 1 && line[0].equals_ci("User-Agent"))
		; // Eventually track user agents to see what everyone is using!
	}
	// CONTENT! :D
	else
	{
	    httpcontent.push_back(message);

	    if(message.equals_ci("</message>"))
	    {
		Flux::string reply =
		"<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
		"<methodResponse>\n"
		"<params>\n"
		"<param><value><string>Commit Received!</string></value></param>\n"
		"</params>\n"
		"</methodResponse>\n";


// 		for(unsigned i = 0; i < httpcontent.size(); ++i)
// 		{
// 		    Log(LOG_TERMINAL) << "Content: " << httpcontent[i];
// 		    contentbytes += httpcontent[i].size();
// 		}

		for(unsigned i = 0; i < httpheader.size(); ++i)
		    if(httpheader[i].search_ci("Content-Length"))
			Log(LOG_TERMINAL) << httpheader[i];

		Log(LOG_TERMINAL) << "Content Bytes: " << contentbytes;

		if(this->HandleMessage())
		    this->HTTPReply(200, "OK", "text/xml", reply);
		else
		    this->HTTPReply(500, "Internal server error", "", "");
		return false;
	    }
	}

	this->ProcessWrite();
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

	this->ProcessWrite();
    }

    bool ProcessWrite()
    {
    //     if(!this->WriteBuffer.empty())
    //       Log(LOG_TERMINAL) << "Process Write:\n " << this->WriteBuffer;
	return BufferedSocket::ProcessWrite() && ClientSocket::ProcessWrite();
    }

    bool HandleMessage();
    void Tick(time_t)
    {
	Log(LOG_DEBUG) << "[XML-RPC] Connection Timeout for " << this->clientaddr.addr() << ", closing connection.";
	this->HTTPReply(408, "Request Timeout", "", "");
    }
};

ClientSocket *xmlrpclistensocket::OnAccept(int fd, const sockaddrs &addr)
{
    // TODO: Reverse DNS Resolve the IP address for logging?
    DNSResolver *res = new DNSResolver(addr.addr());
    res->Process();
    ClientSocket *c = new xmlrpcclient(this, fd, addr);
    return c;
}

/* This is down here so we don't make a huge mess in the middle of the file */
// Parse our message then announce it as a commit using the OnCommit event.
bool xmlrpcclient::HandleMessage()
{

    if(this->httpcontent.empty())
	return false;

    Log(LOG_TERMINAL) << "[XML-RPC] Message Handler Called!";
    Flux::string blah = CondenseVector(this->httpcontent).cc_str();

    // Strip out all the XML garbage we don't need since RapidXML will crash if we don't
    size_t pos1 = blah.find("<?");
    size_t pos2 = blah.find("<message>");

    blah = blah.erase(pos1, pos2).replace_all_cs("  ", " ");

    try
    {
	// Our message
	CommitMessage message;
	// Parse the XML
	rapidxml::xml_document<> doc;
	doc.parse<0>(blah.cc_str());

	// Attempt to get the first node of the commit.
	rapidxml::xml_node<> *main_node = doc.first_node("message", 0, true);
	if(!main_node)
	{
	    Log(LOG_TERMINAL) << "Invalid XML data!";
	    return false;
	}

	/* message.generator section */
	rapidxml::xml_node<> *node = main_node->first_node("generator", 0, true);
	if(node)
	{
	    if(node->first_node("name", 0, true))
		message.info["ScriptName"] = node->first_node("name", 0, true)->value();
	    if(node->first_node("version", 0, true))
		message.info["ScriptVersion"] = node->first_node("version", 0, true)->value();
	    if(node->first_node("url", 0, true))
		message.info["ScriptURL"] = node->first_node("url", 0, true)->value();
	    if(node->first_node("author", 0, true))
		message.info["ScriptAuthor"] = node->first_node("author", 0, true)->value();
	}

	/* message.source section */
	node = main_node->first_node("source", 0, true);
	if(node)
	{
	    if(node->first_node("project", 0, true))
		message.info["project"] = node->first_node("project", 0, true)->value();
	    if(node->first_node("branch", 0, true))
		message.info["branch"] = node->first_node("branch", 0, true)->value();
	    if(node->first_node("module", 0, true))
		message.info["module"] = node->first_node("module", 0, true)->value();
	}
	else
	    return false;

	/* message.timestamp section */
	if(main_node->first_node("timestamp", 0, true))
	    message.info["timestamp"] = main_node->first_node("timestamp", 0, true)->value();

	/* message.body.commit section */
	rapidxml::xml_node<> *mnode = main_node->first_node("body", 0, true);
	if(mnode)
	{
	    node = mnode->first_node("commit", 0, true);
	    if(node)
	    {
		if(node->first_node("author", 0, true))
		    message.info["author"] = node->first_node("author", 0, true)->value();
		if(node->first_node("revision", 0, true))
		    message.info["revision"] = node->first_node("revision", 0, true)->value();
		if(node->first_node("log", 0, true))
		    message.info["log"] = node->first_node("log", 0, true)->value();

		/* message.body.commit.files section */
		if(node->first_node("files", 0, true) && node->first_node("files", 0, true)->first_node("file", 0, true))
		{
		    // Put our children in a vector, damn kids.. packing into vectors nowadays, i remember when
		    // it was cool to pack into hashes!
		    for (rapidxml::xml_node<> *child = node->first_node("files", 0, true)->first_node("file", 0, true); child; child = child->next_sibling())
			message.Files.push_back(child->value());

		    // Sort our messages and remove any duplicates.
		    std::sort(message.Files.begin(), message.Files.end());
		    message.Files.erase(std::unique(message.Files.begin(), message.Files.end()), message.Files.end());
		}
	    }

	    node = mnode->first_node("stats", 0, true);
	    if(node)
	    {
		if(node->first_node("insertions", 0, true))
		    message.info["insertions"] = node->first_node("insertions", 0, true)->value();
		if(node->first_node("deletions", 0, true))
		    message.info["deletions"] = node->first_node("deletions", 0, true)->value();
	    }
	}

	// remove everything from memory
	doc.clear();

	for(auto IT : Networks)
	{
	    for(auto it : IT.second->ChanMap)
		message.Channels.push_back(it.second);
	}

	Log(LOG_TERMINAL) << "\n*** COMMIT INFO! ***";
	for(auto it : message.info)
	    Log(LOG_TERMINAL) << it.first << ": " << it.second;

	int i = 0;
	for(auto it : message.Files)
	    Log(LOG_TERMINAL) << "File[" << ++i << "]: " << it;

	i = 0;
	for(auto it : message.Channels)
	    Log(LOG_TERMINAL) << Flux::Sanitize("Channel["+ value_cast<Flux::string>(++i) + "]: " + it->name + " - " + it->n->name + " | @") << it;

	Log(LOG_TERMINAL) << "*** END COMMIT INFO! ***\n";

	/* Announce to other modules for commit announcement */
	FOREACH_MOD(I_OnCommit, OnCommit(message));
    }
    catch (std::exception &ex)
    {
	Log(LOG_TERMINAL) << "XML Exception Caught: " << ex.what();
	return false;
    }
    return true;
}

class RetryStart : public Timer
{
public:
    RetryStart():Timer(10, time(NULL), false) {}
    void Tick(time_t)
    {
	try
	{
	    new xmlrpclistensocket(Config->xmlrpcbindip, Config->xmlrpcport, Config->xmlrpcipv6);
	}
	catch(const SocketException &ex)
	{
	    Log() << "[XML-RPC] " << ex.GetReason();
	    new RetryStart();
	}
    }
};

class xmlrpcmod : public Module
{
public:
    xmlrpcmod(const Flux::string &Name) : Module(Name, MOD_NORMAL)
    {
	this->SetAuthor("Justasic");
	this->SetVersion(VERSION);
	me = this;

	// Try and make a socket.
	new RetryStart();
    }

    ~xmlrpcmod()
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

	for(std::vector<xmlrpclistensocket*>::iterator it = listen_sockets.begin(), it_end = listen_sockets.end(); it != it_end; ++it)
	    delete *it;
    }
};

MODULE_HOOK(xmlrpcmod)
