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
"<p>This will not provide XML-RPC requests and ONLY uses POST to commit the data (same as most <a href=\"http://cia.vc/doc/clients/\">CIA.vc scripts</a> work), if you are looking for the site, please see <a href=\"http://www.Flux-Net.net/\">Flux-Net.net</a> for the sites location or optionaly connect to Flux-Net IRC for support:</p>\n"
"<a href=\"irc://irc.flux-net.net/Computers\"><FONT COLOR=\"red\">irc.flux-net.net</FONT>:<FONT COlOR=\"Blue\">6667</FONT></a></br>\n"
"Channel: <FONT COLOR=\"Green\">#Commits</FONT></br>\n"
"Channel: <FONT COLOR=\"Green\">#Computers</FONT></br></center>\n";

class WaitTimer : public Timer
{
  Socket *s;
public:
  WaitTimer(Socket *ss, time_t timeout = Config->xmlrpctimeout):Timer(timeout, time(NULL)), s(ss)
  { Log(LOG_TERMINAL) << "WAIT TIMER!"; }
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
  Flux::vector FilesXML;
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
      this->RawCommitXML += message.strip();
    }
    else if(this->in_query)
    {
      Log(LOG_DEBUG) << "[XML-RPC] " << message;
      if(message.search_ci("<file>"))
      {
	this->RawCommitXML += message.strip();
	FilesXML.push_back(message.strip());
      }
      
      if(!message.search_ci("</message>"))
	  this->RawCommitXML += message.strip();
      else{
	this->in_query = false;
	this->RawCommitXML += message.strip();
	Log(LOG_DEBUG) << "[XML-RPC] Processing Message from " << GetPeerIP(this->GetFD());
	this->Write("HTTP/1.0 200 OK");
	this->Write("SERVER: ANT Commit System version " + systemver);
	this->Write("CONTENT-LENGTH: 0");
	this->Write("CONNECTION: CLOSE");
	this->Write("DATE: "+do_strftime(time(NULL), true));
	this->Write("");
	this->HandleMessage();
	this->ProcessWrite();
	//return false; //Close the connection.
	new WaitTimer(this, 5);
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
    if(!this->WriteBuffer.empty())
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

class module;
/* This is down here so we don't make a huge mess in the middle of the file */
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
  Log(LOG_TERMINAL) << "\nBLAH! " << blah;

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
	  // Because it seems you cannot parse nodes with same names in RapidXML, we have to make a quick hack
	  for(auto it : this->FilesXML)
	  {
	    Flux::string FileXML = it;
	    rapidxml::xml_document<> doc2;
	    doc2.parse<0>(FileXML.cc_str());
	    
	    if(doc2.first_node("file", 0, true))
	      message.Files.push_back(doc2.first_node()->value());
	  }
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
    Log(LOG_TERMINAL) << ":::: XML Exception Caught: " << ex.what();
  }

// This crap below is kept as example code for when i need to reference something while i develop the handler
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
 
//   Log(LOG_TERMINAL) << "***Commit****\nScriptName: " << ScriptName << "\nScriptVersion: " << ScriptVersion << "\nScriptURL: " << ScriptURL << "\nTimestamp: " << timestamp << "\nAuthor: " << author << "\nRevision: " << revision << "\nLog: " << log << "\nURL: " << url << "\nProject: " << project << "\nBranch: " << branch << "\nModule: " << module << "\n***End Of Commit***";


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

//IRC Colors
#define BLACK "\0031"
#define DARK_BLUE "\0032"
#define DARK_GREEN "\0033"
#define GREEN "\0033"
#define RED "\0034"
#define LIGHT_RED "\0034"
#define DARK_RED "\0035"
#define PURPLE "\0036"
#define BROWN "\0037"
#define ORANGE "\0037"
#define YELLOW "\0038"
#define LIGHT_GREEN "\0039"
#define AQUA "\00310"
#define LIGHT_BLUE "\00311"
#define BLUE "\00312"
#define VIOLET "\00313"
#define GREY "\00314"
#define GRAY "\00314"
#define LIGHT_GREY "\00315"
#define LIGHT_GRAY "\00315"
#define WHITE "\00316"

//Other formatting
#define NORMAL "\17"
#define BOLD "\2"
#define REVERSE ""
#define UNDERLINE "\13"

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

private:
  CommitMessage Message;
  Flux::string GetCommitData(const Flux::string &n)
  {
    CommitMessage msg = this->Message;
    for(auto it : msg.info)
    {
      if(it.first.equals_ci(n))
	return it.second;
    }
    return "";
  }

  // FIXME: This needs some serious fixing! It should calculate directories and files the same as CIA.vc 
  Flux::string BuildFileString(Flux::vector files)
  {
    Flux::string ret;
    if(files.size() <= 3)
    {
      for(auto it : files)
      {
        Flux::string file = it;
        size_t slash = file.rfind("/");
	if(slash < file.size())
	{
	  Log(LOG_TERMINAL) << "File: " << file << " slash: " << slash;
	  Flux::string f = file.substr(slash+1);
	  ret += f + " ";
	}else
	  ret += file + " ";
      }
    } else {
      int dirs = 0;
      for(auto it : files)
      {
	Flux::string file = it;
	size_t slash = file.rfind("/");
	if(slash < file.size())
	{
	  Flux::string dir = file.substr(0, slash);
	  Log(LOG_TERMINAL) << "DIR: " << dir;
	  if(!dir.empty())
	    dirs++;
	}
      }
      std::stringstream ss;
      ss << "(" << files.size() << " files" << (dirs < 0?"":" in "+dirs) << " changed)";
      //ret = "(" + value_cast<Flux::string>(files.size()) + " files" + (dirs < 0?"":" in "+dirs) + " changed)";
      ret = ss.str();
    }
    ret.trim();
    return ret;
  }

public:
  void OnCommit(CommitMessage &msg)
  {
    this->Message = msg;
    // FIXME: if they're no connections, buffer the message
    Log(LOG_DEBUG) << "AnnounceCommit Called.";

    // Calculate files to announce.
    // FIXME: This needs to calculate directories as well
    // FIXME: This needs to be handled by the Rulesets system later on.
    
    Flux::string files = BuildFileString(msg.Files);
    //if(msg.Files.size() <= 2)
    //  files = CondenseVector(msg.Files);
    //else
    //  files = "(" + value_cast<Flux::string>(msg.Files.size()) + " files changed)";
    //files.trim();
    
    for(auto it : msg.Channels)
    {
      Channel *c = it;
      //     Flux::string files = CondenseVector(msg.Files);
      Log(LOG_TERMINAL) << "Announcing in " << c->name << " (" << c->n->name << ')';

      // Build the commit message with stringstream
      std::stringstream ss;
      ss << RED << BOLD << this->GetCommitData("project") << ": " << NORMAL << ORANGE << this->GetCommitData("author") << " * ";
      ss << NORMAL << BOLD << '[' << this->GetCommitData("branch") << "] " << NORMAL << YELLOW << 'r' << this->GetCommitData("revision");
      ss << NORMAL << BOLD << " | " << NORMAL << AQUA << files << NORMAL << ": " << this->GetCommitData("log"); //<< files;
      
      Flux::string formattedmessgae = Flux::string(ss.str()).replace_all_cs("\"", "").replace_all_cs("\n", "").replace_all_cs("\r", "");
      
      //Log(LOG_TERMINAL) << "Commit Msg: \"" <<  formattedmessgae << "\"";
      c->SendMessage(formattedmessgae);
    }
  }
};

MODULE_HOOK(xmlrpcmod)
