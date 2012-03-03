/* Arbitrary Navn Tool -- Bot Functions and Commit Parsing.
 * 
 * (C) 2011-2012 Flux-Net
 * Contact us at Dev@Flux-Net.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

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
#define NORMAL "\15"
#define BOLD "\2\2"
#define REVERSE ""
#define UNDERLINE "\13\13"

// enum IRCColors
// {
//   // Colors
//   BLACK_,
//   WHITE_,
//   DARK_BLUE_,
//   DARK_GREEN_,
//   DARK_RED_,
//   LIGHT_BLUE_,
//   LIGHT_GRAY_,
//   LIGHT_GREEN_,
//   LIGHT_GREY_,
//   LIGHT_RED_,
//   GREEN_,
//   RED_,
//   PURPLE_,
//   YELLOW_,
//   GRAY_,
//   GREY_,
//   BROWN_,
//   ORANGE_,
//   AQUA_,
//   BLUE_,
//   VIOLET_,
// 
//   // Other formatting
//   NORMAL_,
//   BOLD_,
//   REVERSE_,
//   UNDERLINE_,
// }

class RenameTimer : public Timer
{
  Bot *b;
public:
  RenameTimer(time_t wait, Bot *bot):Timer(wait, time(NULL), true), b(bot) {}
  void Tick(time_t);
};

Bot::Bot(Network *net, const Flux::string &ni, const Flux::string &i, const Flux::string &real): User(net, ni, i, net->hostname, real), network(net)
{
  if(!net)
    throw CoreException("Bot with no network??");
  if(ni.empty())
    throw CoreException("Bot with no nickname??");
  if(i.empty())
    throw CoreException("Bot with no ident??");
  if(net->b){
    Log() << "Bot assigned to a network with a bot already assigned??";
    return; //close the constructor instead of throwing.
//     throw CoreException("Bot assigned to a network with a bot already assigned??");
  }

  new RenameTimer(30, this);
  this->n->b = this;
  new IRCProto(this->n);
  Log(LOG_DEBUG) << "New bot created on " << net->name << ": " << ni << i << ": " << real;
}

Bot::~Bot()
{
  this->n->b = NULL;
  delete this->ircproto;
  Log(LOG_DEBUG) << "Bot " << this->nick << " for network " << this->n->name << " deleted!";
}

// void Bot::AnnounceCommit(CommitMessage &msg)
// {
//   Log(LOG_DEBUG) << "AnnounceCommit Called.";
//   for(auto it : msg.Channels)
//   {
//     Channel *c = it;
// //     Flux::string files = CondenseVector(msg.Files);
//     Log(LOG_TERMINAL) << "Announcing in " << c->name << " (" << c->n->name << ')';
//     
//     std::stringstream ss;
//     ss << RED << BOLD << msg.project << ": " << NORMAL << ORANGE << msg.author << " * " << NORMAL << YELLOW << 'r' <<  msg.revision << NORMAL << BOLD << " | " << NORMAL << LIGHT_BLUE << ":\15 " << << msg.log; //<< files;
//     Log(LOG_DEBUG) << "BLAH! " << ss.str();
// 
//     Flux::string formattedmessgae = Flux::string(ss.str()).replace_all_cs("\"", "").replace_all_cs("\n", "").replace_all_cs("\r", "");
// 
//     //Log(LOG_TERMINAL) << "Commit Msg: \"" <<  formattedmessgae << "\"";
//     c->SendMessage(formattedmessgae);
// //     c->SendMessage(RED+BOLD+"%s: "+NORMAL+ORANGE+"%s * "+NORMAL+YELLOW+"%s "+NORMAL+BOLD+"| "+NORMAL+LIGHT_BLUE+"%s"+NORMAL+": %s", msg.project.c_str(), msg.author.c_str(), msg.revision.c_str(), files.c_str());
//   }
// }

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

void Bot::SetMode(const Flux::string &modestr)
{
  if(this->ircproto)
    this->ircproto->mode(this->nick, modestr);
}

void Bot::CheckNickName(const Flux::string &ni)
{
  Flux::string nickname = ni.empty()?this->nick:ni;
  if(this->n->s && this->n->s->GetStatus(SF_CONNECTED))
  {
    unsigned num = 0;
    if(nickname.search(Config->NicknamePrefix))
    {
      //Log(LOG_DEBUG) << "1: " << nickname;
      Flux::string end = nickname.substr(Config->NicknamePrefix.size());
      //Log(LOG_DEBUG) << "2: " << end << "|" << end.size();
      if(end.is_pos_number_only() && end.size() < 10){
	num = (unsigned)end;
	//Log(LOG_DEBUG) << "3: " << num;
      }
    }
    //Log(LOG_TERMINAL) << num;
    if((num <= 0))
      this->SetNick(Config->NicknamePrefix+value_cast<Flux::string>(++num));
  }
}

void Bot::Part(Channel *c, const Flux::string &message)
{
  if(!c->finduser(this->n, this->nick))
    return;

  c->SendPart(message);
}

void Bot::Quit(const Flux::string &message)
{
  this->ircproto->quit(message);
}

void Bot::SetNick(const Flux::string &nickname)
{
  this->SetNewNick(nickname);
  this->ircproto->nick(nickname);
}

void Bot::SendUser()
{
  this->ircproto->user(this->ident, this->realname);
  this->ircproto->nick(this->nick);
}

void RenameTimer::Tick(time_t)
{
//   Log(LOG_TERMINAL) << "RenameTimer Tick.";
  if(b->n->s && b->n->s->GetStatus(SF_CONNECTED))
    b->CheckNickName();
}
