/*
 * This is the module file for the XML_RPC commits, this will handle 98% of the commits the bot will process!
 */
#include "flux_net_irc.hpp"
#include "module.h"

// Simple web page incase a web browser decides to go to the link
const Flux::string HTTPREQUEST = "<center><h1>ANT Commit system version %s</h1></center>"
"<center><h4>This is the address for XML-RPC commits</h4>"
"<p>This will not provide XML-RPC requests and ONLY uses POST to commit the data (same as most <a href=\"http://cia.vc/doc/clients/\">CIA.vc scripts</a> work), if you are looking for the site, please see <a href=\"http://www.Flux-Net.net/\">Flux-Net.net</a> for the sites location or optionaly connect to Flux-Net IRC for support:</p>"
"<a href=\"irc://irc.flux-net.net/Computers\"><FONT COLOR=\"red\">irc.flux-net.net</FONT>:<FONT COlOR=\"Blue\">6667</FONT></a></br>"
"Channel: <FONT COLOR=\"Green\">#Commits</FONT></br>"
"Channel: <FONT COLOR=\"Green\">#Computers</FONT></br></center>";

class xmlrpcclient;
class xmlrpclistensocket;
std::vector<xmlrpclistensocket*> listen_sockets;

class xmlrpclistensocket : public ListenSocket
{
public:
  xmlrpclistensocket(const Flux::string &bindip, int port, bool ipv6) : ListenSocket(bindip, port, ipv6)
  {
    Log(LOG_DEBUG) << "[XML-RPC] New Listen socket created " << bindip << ':' << port << (ipv6?"(IPv6)":"(IPv4)");
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

class xmlrpcclient : public ClientSocket, public BufferedSocket
{
  Flux::string RawCommitXML;
  bool in_query;
public:
  xmlrpcclient(xmlrpclistensocket *ls, int fd, const sockaddrs &addr) : Socket(fd, ls->IsIPv6()), ClientSocket(reinterpret_cast<ListenSocket*>(ls), addr), BufferedSocket() {}
  bool Read(const Flux::string &message)
  {
    if(message.search_ci("USER")) // If the user tries to connect via IRC protocol
      this->Write("ERROR: :Closing link: (unknown@%s) This is not an IRC connection", GetPeerIP(this->GetFD()).c_str());
    else if(message.search_ci("GET") && message.search_ci("http/1.1"))
    { //If connection is HTTP GET request
    const Flux::string page = fsprintf(HTTPREQUEST, VERSION_FULL);
    this->Write("HTTP/1.0 200 OK");
    this->Write("CONNECTION: CLOSE");
    this->Write("CONTENT-TYPE: TEXT/HTML");
    this->Write("CONTENT-LENGTH: " + value_cast<Flux::string>(page.length()));
    this->Write("SERVER: ANT Commit System version " + value_cast<Flux::string>(VERSION_FULL));
    this->Write("");
    this->Write(page);
    return true;
    }else if(message.search_ci("<message>") || this->in_query){ //This is a commit
      Log(LOG_DEBUG) << "[XML-RPC] " << message;
      this->RawCommitXML += message.strip();
    }else if(message.search_ci("</message>")){
      Log(LOG_DEBUG) << "[XML-RPC] Processing Message";
      this->HandleMessage();
    }
    return true;
  }
  bool GetData(Flux::string&, Flux::string&);
  void HandleMessage();
};

ClientSocket *xmlrpclistensocket::OnAccept(int fd, const sockaddrs &addr)
{
  xmlrpcclient *socket = new xmlrpcclient(this, fd, addr);
  
  if(socket->IsDead())
    Log() << "[XML-RPC] Found dead socket " << fd << " from " << addr.addr();
  
  return socket;
}
class module;
/* This is down here so we don't make a huge mess in the middle of the file */
void xmlrpcclient::HandleMessage()
{
  if(this->RawCommitXML.empty())
    return;
  
  XMLFile xf(this->RawCommitXML, 1);
  
  /* This code was based off the commit in this script: http://cia.vc/clients/git/ciabot.bash */
  /* Script Info (if available) */
  Flux::string ScriptName = xf.Tags["message"].Tags["generator"].Attributes["name"].Value;
  Flux::string ScriptVersion = xf.Tags["message"].Tags["generator"].Attributes["version"].Value;
  Flux::string ScriptURL = xf.Tags["message"].Tags["generator"].Attributes["url"].Value;
  
  /* Commit Body */
  Flux::string timestamp = xf.Tags["message"].Attributes["timestamp"].Value;
  Flux::string author = xf.Tags["message"].Tags["body"].Tags["commit"].Attributes["author"].Value;
  Flux::string revision = xf.Tags["message"].Tags["body"].Tags["commit"].Attributes["revision"].Value;
  Flux::string log = xf.Tags["message"].Tags["body"].Tags["commit"].Attributes["log"].Value;
  Flux::string url = xf.Tags["message"].Tags["body"].Tags["commit"].Attributes["url"].Value;
  Flux::string files = xf.Tags["message"].Tags["body"].Tags["commit"].Attributes["files"].Value;
  
  /* Source info */
  Flux::string project = xf.Tags["message"].Tags["source"].Attributes["project"].Value;
  Flux::string branch = xf.Tags["message"].Tags["source"].Attributes["branch"].Value;
  Flux::string module = xf.Tags["message"].Tags["source"].Attributes["module"].Value;
  
  sepstream sep(files, ' ');
  Flux::string tok;
  Flux::vector Files;
  while(sep.GetToken(tok))
    Files.push_back(tok);
  
  /* This is seperated now to keep
   * the differences in how stuff is
   * processed apart so we can possibly
   * the data in more ways than one
   */
  
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
  msg.Files = Files;
  
  /* Source info */
  msg.project = project;
  msg.branch = branch;
  msg.module = module;
  
  /* Announce to other modules for commit announcement */
  FOREACH_MOD(I_OnCommit, OnCommit(msg));
}

class xmlrpcmod : public module
{
public:
  xmlrpcmod(const Flux::string &Name):module(Name)
  {
    this->SetAuthor("Justasic");
    this->SetVersion(VERSION);
    Implementation i[] = { I_OnCommit };
    ModuleHandler::Attach(i, this, sizeof(i)/sizeof(Implementation));
    xmlrpclistensocket *xmll = new xmlrpclistensocket("127.0.0.1", 12345, false);
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
    Log(LOG_TERMINAL) << "[XML-RPC] Fun stuff!";
  }
};

MODULE_HOOK(xmlrpcmod)