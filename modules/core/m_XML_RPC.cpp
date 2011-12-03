/*
 * This is the module file for the XML_RPC commits, this will handle 98% of the commits the bot will process!
 */
#include "flux_net_irc.hpp"

// Simple web page incase a web browser decides to go to the link
constexpr Flux::string HTTPREQUEST = "<center><h1>ANT Commit system version %s</h1></center>"
"<center><h4>This is the address for XML-RPC commits</h4>"
"<p>This will not provide XML-RPC requests and ONLY uses POST to commit the data (same as most <a href=\"http://cia.vc/doc/clients/\">CIA.vc scripts</a> work), if you are looking for the site, please see <a href=\"http://www.Flux-Net.net/\">Flux-Net.net</a> for the sites location or optionaly connect to Flux-Net IRC for support:</p>"
"<a href=\"irc://irc.flux-net.net/Computers\"><FONT COLOR=\"red\">irc.flux-net.net</FONT>:<FONT COlOR=\"Blue\">6667</FONT></a></br>"
"Channel: <FONT COLOR=\"Green\">#Commits</FONT></br>"
"Channel: <FONT COLOR=\"Green\">#Computers</FONT></br></center>";

class xmlrpcclient;
class xmlrpclistensocket;
std::vector<xmlrpclistensocket*> listen_sockets;

CommitMessage ProcessCommit(const Flux::string &str)
{
  
}

class xmlrpcclient : public ClientSocket, public BufferedSocket
{
  Flux::string RawCommitXML;
  
public:
  xmlrpcclient(xmlrpclistensocket *ls, int fd, const sockaddrs &addr) : Socket(fd, ls->IsIPv6())ClientSocket(ls, addr), BufferedSocket() {}
  bool Read(const Flux::string&)
  {
    if(message.search_ci("USER")) // If the user tries to connect via IRC protocol
      this->Write("ERROR: This is not an IRC connection");
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
    }else if(message.search_ci("xml version")){ //This is a commit
      static_cast<void>(0); //Do nothing for now...
    }
    return true;
  }
  bool GetData(Flux::string&, Flux::string&);
  void HandleMessage();
};

class xmlrpclistensocket : public ListenSocket
{
public:
  xmlrpclistensocket(const Flux::string &bindip, int port, bool ipv6) : ListenSocket(bindip, port, ipv6)
  {
    log(LOG_DEBUG) << "New Listen socket created " << bindip << ':' << port << (ipv6?"(IPv6)":"(IPv4)");
    listen_sockets.push_back(this);
  }
  ~xmlrpclistensocket()
  {
    auto it = std::find(listen_sockets.begin(), listen_sockets.end(), this);
    if(it != listen_sockets.end())
      listen_sockets.erase(it);
  }
  ClientSocket *OnAccept(int fd, const sockaddrs &addr)
  {
    xmlrpcclient *socket = new xmlrpcclient(this, fd, addr);

    if(socket->IsDead())
      Log() << "XML-RPC: Found dead socket " << fd << " from " << addr.addr();
    
    return socket;
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
      delete (*it);
  }
  void OnCommit(CommitMessage &msg)
  {
    Log(LOG_TERMINAL) << "Fun stuff!";
  }
};