#include "bot.h"

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
#define NORMAL "\15\15"
#define BOLD "\2\2"
#define REVERSE ""
#define UNDERLINE "\13\13"

Bot::Bot(Network *net, const Flux::string &ni, const Flux::string &i, const Flux::string &real): User(net, ni, i, net->hostname, real), network(net), nick(ni), ident(i), realname(real)
{
  if(!net)
    throw CoreException("Bot with no network??");
  if(ni.empty())
    throw CoreException("Bot with no nickname??");
  if(i.empty())
    throw CoreException("Bot with no ident??");
  
  this->n->bots[ni] = this;
  Log(LOG_DEBUG) << "New bot created on " << net->name << ": " << n << i << ": " << real;
}

Bot::~Bot()
{
  this->n->bots.erase(this->nick);
  Log(LOG_DEBUG) << "Bot " << this->nick << " for network " << this->n->name << " deleted!";
}

void Bot::AnnounceCommit(CommitMessage &msg)
{
  for(auto it : msg.Channels)
  {
    Channel *c = it;
    Flux::string files = CondenseVector(msg.Files);
    std::stringstream ss;
    ss << RED << BOLD << msg.project+" " << NORMAL << ORANGE << msg.author << " * " << NORMAL << YELLOW << msg.revision << NORMAL << BOLD << " | " << NORMAL << LIGHT_BLUE << files;
    c->SendMessage(ss.str());
//     c->SendMessage(RED+BOLD+"%s: "+NORMAL+ORANGE+"%s * "+NORMAL+YELLOW+"%s "+NORMAL+BOLD+"| "+NORMAL+LIGHT_BLUE+"%s"+NORMAL+": %s", msg.project.c_str(), msg.author.c_str(), msg.revision.c_str(), files.c_str());
  }
}

void Bot::Join(Channel *c)
{
  if(c)
    c->SendJoin();
}

void Bot::Join(const Flux::string &chan)
{
  Channel *c = FindChannel(this->n, chan);
  if(!c)
    c = new Channel(this->n, chan);
  c->SendJoin();
}

void Bot::Part(Channel *c, const Flux::string &message)
{
  if(!c->finduser(this->nick))
    return;

  c->SendPart(message);
}

void Bot::Quit(const Flux::string &message)
{
  this->n->ircproto->quit(message);
}

void Bot::SetNick(const Flux::string &)
{
  //FIXME: this needs fixing
}

void Bot::SendUser()
{
  this->n->ircproto->user(this->ident, this->realname);
  this->n->ircproto->nick(this->nick);
}