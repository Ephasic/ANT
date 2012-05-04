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
#include "module.h"
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
    chars("", "")
  };
  
  Flux::string ret = str;
  for(int i = 0; special[i].character.empty() == false; ++i)
    ret = ret.replace_all_cs(special[i].character, special[i].replace);
  return ret;
}

// Simple web page incase a web browser decides to go to the link
const Flux::string systemver = value_cast<Flux::string>(VERSION_FULL);
const Flux::string HTTPREQUEST = "<center><h1>ANT Commit system version "+systemver+"</h1></center>\n"
"<center><h4>This is the address for XML-RPC commits</h4>\n"
"<p>This will not provide XML-RPC requests and ONLY uses POST to commit the data (same as most <a href=\"http://cia.vc/doc/clients/\">CIA.vc scripts</a> work), if you are looking for the site, please see <a href=\"http://www.Azuru.net/\">Azuru.net</a> for the sites location or optionally connect to Azuru IRC for support:</p>\n"
"<a href=\"irc://irc.Azuru.net/Computers\"><FONT COLOR=\"red\">irc.Azuru.net</FONT>:<FONT COlOR=\"Blue\">6667</FONT></a></br>\n"
"Channel: <FONT COLOR=\"Green\">#Commits</FONT></br>\n"
"Channel: <FONT COLOR=\"Green\">#Computers</FONT></br></center>\n";

class xmlrpcclient;
class xmlrpclistensocket;
std::vector<xmlrpclistensocket*> listen_sockets;

/*****************************************************************/
/*********************** Listen Socket ***************************/
/*****************************************************************/
class xmlrpclistensocket : public ListenSocket
{
public:
  xmlrpclistensocket(const Flux::string &bindip, int port, bool ipv6) : ListenSocket(bindip, port, ipv6)
  {
    Log(LOG_DEBUG) << "[XML-RPC] New Listen socket created " << bindip << ':' << port << (ipv6?" (IPv6)":" (IPv4)");
    listen_sockets.push_back(this);
  }
  ~xmlrpclistensocket()
  {
    auto it = std::find(listen_sockets.begin(), listen_sockets.end(), this);
    if(it != listen_sockets.end())
      listen_sockets.erase(it);
  }
  ClientSocket *OnAccept(int fd, const sockaddrs &addr);
};
/*****************************************************************/
/*********************** Client Socket ***************************/
/*****************************************************************/

// Declared below
E Flux::string messagestr;
E xmlrpcclient *client;
E void htmlcall();

class xmlrpcclient : public ClientSocket, public BufferedSocket, public Timer
{
  Flux::string RawCommitXML;
  bool in_query, in_header, IsXML, is_httpreq;
public:
  xmlrpcclient(xmlrpclistensocket *ls, int fd, const sockaddrs &addr) : Socket(fd, ls->IsIPv6()),
  ClientSocket(reinterpret_cast<ListenSocket*>(ls), addr), BufferedSocket(), Timer(Config->xmlrpctimeout),
  in_query(false), in_header(false), IsXML(false), is_httpreq(false) {}

  bool Read(const Flux::string &m)
  {
    Flux::string message = SanitizeXML(m);
    Log(LOG_TERMINAL) << "Message: \"" << message << "\"";
    
    if(message.search_ci("GET") && message.search_ci("HTTP/1."))
    { //If connection is HTTP GET request
      messagestr = message;
      client = this;
      new tqueue(htmlcall, 0); // Wait for the 3 second timeout
      this->is_httpreq = true;
      Log(LOG_TERMINAL) << "HTTP GET Request.";
      return true;
    }
    else if((message.search_ci("POST") && message.search_ci("HTTP/1.")))
    { //This is a commit
      this->in_header = true;
      Log(LOG_DEBUG) << "[XML-RPC] " << message;
    }
    else if(this->in_header && (message.search_ci("Content-Type: text/xml") || message.search_ci("Content Type: text/xml")))
      this->IsXML = true;
    else if(message.search_ci("<?xml"))
    {
      this->in_query = this->IsXML = true;
      this->in_header = false;
      this->RawCommitXML += message.strip();
    }
    else if(this->in_query)
    {
      Log(LOG_DEBUG) << "[XML-RPC] " << message;

      if(!message.search_ci("</message>"))
	  this->RawCommitXML += message.strip();
      else
      {
	this->in_query = false;
	this->RawCommitXML += message.strip();
	Log(LOG_DEBUG) << "[XML-RPC] Processing Message from " << this->clientaddr.addr();

	// Reply to our XML-RPC request stating that we received the commit.
	Flux::string reply = "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
	"<methodResponse>\n"
	"<params>\n"
	"<param><value><string>Commit Received!</string></value></param>\n"
	"</params>\n"
	"</methodResponse>\n";
	
	this->Write("HTTP/1.0 200 OK");
	this->Write("SERVER: ANT Commit System version " + systemver);
	this->Write("CONTENT-TYPE: TEXT/XML");
	this->Write(printfify("CONTENT-LENGTH: %i", reply.size()));
	this->Write("CONNECTION: CLOSE");
	this->Write("DATE: "+do_strftime(time(NULL), true));
	this->Write("");
	this->Write(reply);
	this->HandleMessage();
	this->ProcessWrite();
	return false; //Close the connection.
      }
    }
    else if(!this->in_query && !this->in_header && !this->IsXML && !this->is_httpreq)
    {
      Log(LOG_TERMINAL) << "Invalid HTTP POST.";
      return false; //Invalid HTTP POST, not XML-RPC
    }
    else if(this->in_header || this->is_httpreq) //We're still in the header, but don't need the junk.
      return true;
    else
    {
      this->Write("ERROR: Unknown connection from "+this->clientaddr.addr()+"\n");
      this->ProcessWrite(); //Write the data
      Log(LOG_TERMINAL) << "Unknown: " << message;
      return false; //Close the connection.
    }
    this->ProcessWrite();
    return true;
  }
  
  bool ProcessWrite()
  {
    if(!this->WriteBuffer.empty())
      Log(LOG_TERMINAL) << "Process Write:\n " << this->WriteBuffer;
    return BufferedSocket::ProcessWrite() && ClientSocket::ProcessWrite();
  }
  
  void HandleMessage();
  void Tick(time_t)
  {
    Log(LOG_DEBUG) << "[XML-RPC] Connection Timeout for " << this->clientaddr.addr() << ", closing connection.";
    this->SetDead(true);
  }
};

Flux::string messagestr;
xmlrpcclient *client;

void htmlcall()
{
  int len = HTTPREQUEST.size();
  bool isfavicon = messagestr.search_ci("/favicon.ico");
  client->Write("HTTP/1.0 200 OK");
  client->Write("CONNECTION: CLOSE");
  client->Write("CONTENT-TYPE: TEXT/HTML");
  client->Write(printfify("CONTENT-LENGTH: %i", isfavicon?0:len));
  client->Write("DATE: "+do_strftime(time(NULL), true));
  client->Write("SERVER: ANT Commit System version " + systemver);
  client->Write("");
  client->Write(isfavicon?"":HTTPREQUEST);
  client->ProcessWrite();
  client->SetStatus(SF_DEAD, true);
}

ClientSocket *xmlrpclistensocket::OnAccept(int fd, const sockaddrs &addr)
{
  Log(LOG_DEBUG) << "[XML-RPC] Accepting connection from " << addr.addr();
  ClientSocket *c = new xmlrpcclient(this, fd, addr);
  return c;
}

class module;
/* This is down here so we don't make a huge mess in the middle of the file */
// Parse our message then announce it as a commit using the OnCommit event.
void xmlrpcclient::HandleMessage()
{
  if(this->RawCommitXML.empty())
    return;

  Log(LOG_TERMINAL) << "[XML-RPC] Message Handler Called!";
  Flux::string blah = this->RawCommitXML.cc_str();
  // Strip out all the XML garbage we don't need since RapidXML will crash if we don't
  size_t pos1 = blah.find("<?");
  size_t pos2 = blah.find("<message>");

  blah = blah.erase(pos1, pos2).replace_all_cs("  ", " ");
//   Log(LOG_TERMINAL) << "\nBLAH! " << blah;

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
      return;
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
    }

    Log(LOG_TERMINAL) << "\n*** COMMIT INFO! ***";
    for(auto it : message.info)
      Log(LOG_TERMINAL) << it.first << ": " << it.second;

    int i = 0;
    for(auto it : message.Files)
      Log(LOG_TERMINAL) << "File[" << ++i << "]: " << it;
    
    Log(LOG_TERMINAL) << "*** END COMMIT INFO! ***\n";

    for(auto IT : Networks)
    {
      for(auto it : IT.second->ChanMap)
	message.Channels.push_back(it.second);
    }

    /* Announce to other modules for commit announcement */
    FOREACH_MOD(I_OnCommit, OnCommit(message));
  }
  catch (std::exception &ex)
  {
    Log(LOG_TERMINAL) << "XML Exception Caught: " << ex.what();
  }
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

class xmlrpcmod : public module
{
public:
  xmlrpcmod(const Flux::string &Name):module(Name)
  {
    this->SetAuthor("Justasic");
    this->SetVersion(VERSION);

    // Try and make a socket.
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

    for(auto it : listen_sockets)
      delete (it);
  }
};

MODULE_HOOK(xmlrpcmod)
