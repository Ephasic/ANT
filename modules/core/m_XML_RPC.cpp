/* Arbitrary Navn Tool -- XML-RPC Parser Module
 * 
 * (C) 2011-2012 Flux-Net
 * Contact us at Dev@Flux-Net.net
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
"<p>This will not provide XML-RPC requests and ONLY uses POST to commit the data (same as most <a href=\"http://cia.vc/doc/clients/\">CIA.vc scripts</a> work), if you are looking for the site, please see <a href=\"http://www.Flux-Net.net/\">Flux-Net.net</a> for the sites location or optionaly connect to Flux-Net IRC for support:</p>\n"
"<a href=\"irc://irc.flux-net.net/Computers\"><FONT COLOR=\"red\">irc.flux-net.net</FONT>:<FONT COlOR=\"Blue\">6667</FONT></a></br>\n"
"Channel: <FONT COLOR=\"Green\">#Commits</FONT></br>\n"
"Channel: <FONT COLOR=\"Green\">#Computers</FONT></br></center>\n";

class WaitTimer : public Timer
{
  Socket *s;
public:
  WaitTimer(Socket *ss):Timer(Config->xmlrpctimeout, time(NULL)), s(ss) { Log(LOG_TERMINAL) << "WAIT TIMER!"; }
  void Tick(time_t) { s->SetDead(true); }
};

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
class xmlrpcclient : public ClientSocket, public BufferedSocket
{
  Flux::string RawCommitXML;
  Flux::vector XML_VEC;
  bool in_query, in_header, IsXML;
public:
  xmlrpcclient(xmlrpclistensocket *ls, int fd, const sockaddrs &addr) : Socket(fd, ls->IsIPv6()), ClientSocket(reinterpret_cast<ListenSocket*>(ls), addr), BufferedSocket(), in_query(false), IsXML(false) {}
  
  bool Read(const Flux::string &m)
  {
    Flux::string message = SanitizeXML(m);
    Log(LOG_TERMINAL) << "Message: \"" << message << "\"";
    if(message.search_ci("GET") && message.search_ci("HTTP/1."))
    { //If connection is HTTP GET request
      int len = HTTPREQUEST.size();
      bool isfavicon = message.search_ci("/favicon.ico");
      this->Write("HTTP/1.0 200 OK");
      this->Write("CONNECTION: CLOSE");
      this->Write("CONTENT-TYPE: TEXT/HTML");
      this->Write(printfify("CONTENT-LENGTH: %i", isfavicon?0:len));
      this->Write("DATE: "+do_strftime(time(NULL), true));
      this->Write("SERVER: ANT Commit System version " + systemver);
      this->Write(" ");
      this->Write(isfavicon?"":HTTPREQUEST);
      this->ProcessWrite();
      Log(LOG_TERMINAL) << "HTTP GET Request.";
      return true;
    }
    else if((message.search_ci("POST") && message.search_ci("HTTP/1."))) 
    { //This is a commit
      this->in_header = true;
      Log(LOG_DEBUG) << "[XML-RPC] " << message;
    }
    else if(this->in_header && message.search_ci("Content-Type: text/xml"))
      this->IsXML = true;
    else if(message.search_ci("<?xml"))
    {
      this->in_query = this->IsXML = true;
      this->in_header = false;
    }
    else if(this->in_query)
    {
      Log(LOG_DEBUG) << "[XML-RPC] " << message;
      if(!message.search_ci("</message>")){
	this->RawCommitXML += message.strip();
	this->XML_VEC.push_back(message.strip());
      }else{
	this->in_query = false;
	this->RawCommitXML += message.strip();
	this->XML_VEC.push_back(message.strip());
	Log(LOG_DEBUG) << "[XML-RPC] Processing Message from " << GetPeerIP(this->GetFD());
	this->Write("HTTP/1.1 200 OK");
	this->Write("CONNECTION: CLOSE");
	this->Write("DATE: "+do_strftime(time(NULL), true));
	this->Write("SERVER: ANT Commit System version " + systemver);
	this->HandleMessage();
	this->ProcessWrite();
	return false; //Close the connection.
      }
    }
    else if(!this->in_query && !this->in_header && !this->IsXML)
    {
      Log(LOG_TERMINAL) << "Invalid HTTP POST.";
      //return false; //Invalid HTTP POST, not XML-RPC
    }
    else if(this->in_header) //We're still in the header, but don't need the junk.
      return true;
    else
    {
      this->Write("ERROR: Unknown connection from "+GetPeerIP(this->GetFD())+"\n");
      this->ProcessWrite(); //Write the data
      Log(LOG_TERMINAL) << "Unknown: " << message;
      return false; //Close the connection.
    }
    this->ProcessWrite();
    return true;
  }
  bool ProcessWrite()
  {
    Log(LOG_TERMINAL) << "Process Write: " << this->WriteBuffer;
    return BufferedSocket::ProcessWrite() && ClientSocket::ProcessWrite();
  }
  bool GetData(Flux::string&, Flux::string&);
  void HandleMessage();
};

ClientSocket *xmlrpclistensocket::OnAccept(int fd, const sockaddrs &addr)
{
  Log(LOG_DEBUG) << "[XML-RPC] Accepting connection from " << addr.addr();
  ClientSocket *c = new xmlrpcclient(this, fd, addr);
  new WaitTimer(c);
  return c;
}

Flux::string GetXMLValue(const Flux::vector &XML_VEC, const Flux::string &v)
{
  //FIXME: this is all lower case!
  Flux::string value = v.tolower();
  for(unsigned i = 0; i < XML_VEC.size(); ++i)
  {
    try
    {
      rapidxml::xml_document<> doc;
      char *buffer = XML_VEC[i].tolower().cc_str();
      doc.parse<0>(buffer);
      
      //       for(rapidxml::xml_node<>* n = doc.first_node("message"); n; n = n->next_sibling())
      //       {
	// 	Flux::string val = value_cast<Flux::string>(n->value());
      // 	if(!val.empty())
      // 	  Log(LOG_TERMINAL) << n->name() << ": " << val;
      //       }
      if(doc.first_node(value.c_str()) && !Flux::string(doc.first_node(value.c_str())->value()).empty()){
	Flux::string author = doc.first_node(value.c_str())->value();
	Log(LOG_TERMINAL) << value << "! " << author;
	return author;
      }
    }
    catch(std::exception &ex)
    {
      Log(LOG_TERMINAL) << "XML Exception Caught: " << ex.what();
    }
  }
  return "";
}

class module;
/* This is down here so we don't make a huge mess in the middle of the file */
void xmlrpcclient::HandleMessage()
{
  if(this->RawCommitXML.empty())
    return;
  Log(LOG_TERMINAL) << "[XML-RPC] Message Handler Called!";
  Log(LOG_TERMINAL) << "COMMIT!!!!! \"" << this->RawCommitXML << "\"";

  Flux::string ScriptName = "herp"; //GetXMLValue(XML_VEC, "project");
  Flux::string ScriptVersion = "1.0";
  Flux::string ScriptURL = "derp";

  Flux::string timestamp = value_cast<Flux::string>(time(NULL)); //GetXMLValue(XML_VEC, "")
  Flux::string author = GetXMLValue(XML_VEC, "author");
  Flux::string revision = GetXMLValue(XML_VEC, "revision");
  Flux::string log = GetXMLValue(XML_VEC, "log");
  Flux::string url = "herp.com";

  Flux::string project = GetXMLValue(XML_VEC, "project");
  Flux::string branch = GetXMLValue(XML_VEC, "branch");

  Flux::string module = "";
  
//   XMLFile xf(this->RawCommitXML, 1);
//   
//   /* This code was based off the commit in this script: http://cia.vc/clients/git/ciabot.bash */
//   /* Script Info (if available) */
//   Flux::string ScriptName = xf.Tags["message"].Tags["generator"].Attributes["name"].Value;
//   Flux::string ScriptVersion = xf.Tags["message"].Tags["generator"].Attributes["version"].Value;
//   Flux::string ScriptURL = xf.Tags["message"].Tags["generator"].Attributes["url"].Value;
//   
//   /* Commit Body */
//   Flux::string timestamp = xf.Tags["message"].Attributes["timestamp"].Value;
//   Flux::string author = xf.Tags["message"].Tags["body"].Tags["commit"].Attributes["author"].Value;
//   Flux::string revision = xf.Tags["message"].Tags["body"].Tags["commit"].Attributes["revision"].Value;
//   Flux::string log = xf.Tags["message"].Tags["body"].Tags["commit"].Attributes["log"].Value;
//   Flux::string url = xf.Tags["message"].Tags["body"].Tags["commit"].Attributes["url"].Value;
//   auto files = xf.Tags["message"].Tags["body"].Tags["commit"].Tags["files"].Tags;
//   
//   /* Source info */
//   Flux::string project = xf.Tags["message"].Tags["source"].Attributes["project"].Value;
//   Flux::string branch = xf.Tags["message"].Tags["source"].Attributes["branch"].Value;
//   Flux::string module = xf.Tags["message"].Tags["source"].Attributes["module"].Value;
//   
// //   sepstream sep(files, ' ');
// //   Flux::string tok;
// //   Flux::vector Files;
// //   while(sep.GetToken(tok))
// //     Files.push_back(tok);
//   Flux::vector Files; //Parse the file list.
//   for(auto it : files)
//     Files.push_back(it.second.Attributes["file"].Value);
  
  /* This is separated now to keep
   * the differences in how stuff is
   * processed apart so we can possibly
   * the data in more ways than one
   */

  Log(LOG_TERMINAL) << "***Commit****\nScriptName: " << ScriptName << "\nScriptVersion: " << ScriptVersion << "\nScriptURL: " << ScriptURL << "\nTimestamp: " << timestamp << "\nAuthor: " << author << "\nRevision: " << revision << "\nLog: " << log << "\nURL: " << url << "\nProject: " << project << "\nBranch: " << branch << "\nModule: " << module << "\n***End Of Commit***";

  // FIXME: This should be in a map to make invalid XML just announce a blank instead of crashing.
  CommitMessage msg;
  /* Script info */
  msg.ScriptName = ScriptName;
  msg.ScriptVersion = ScriptVersion;
  msg.ScriptURL = ScriptURL;

  /* commit body */
  msg.timestamp = timestamp;
  msg.author = author;
  msg.revision = revision;
  msg.log = log;
  msg.url = url;
//   msg.Files = Files;

  /* Source info */
  msg.project = project;
  msg.branch = branch;
//   msg.module = module;

  for(auto IT : Networks) for(auto it : IT.second->ChanMap)
      msg.Channels.push_back(it.second);

  /* Announce to other modules for commit announcement */
  FOREACH_MOD(I_OnCommit, OnCommit(msg));
}

class SocketStart : public Timer //Weird socket glitch where we need to use a timer to start the socket correctly.
{
public:
  SocketStart():Timer(1, time(NULL), false) {}
  void Tick(time_t)
  {
    try{
      new xmlrpclistensocket(Config->xmlrpcbindip, Config->xmlrpcport, Config->xmlrpcipv6);
    }catch(const SocketException &ex){
      Log() << "[XML-RPC] " << ex.GetReason();
      new SocketStart();
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
    Implementation i[] = { I_OnCommit };
    ModuleHandler::Attach(i, this, sizeof(i)/sizeof(Implementation));
    new SocketStart();
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
  
  void OnCommit(CommitMessage &msg)
  {
    //FIXME: if they're no connections, buffer the message
    Log(LOG_TERMINAL) << "[XML-RPC] Fun stuff!";
    for(auto it : Networks)
      it.second->b->AnnounceCommit(msg);
  }
};

MODULE_HOOK(xmlrpcmod)