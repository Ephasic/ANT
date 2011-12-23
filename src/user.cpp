/* All code is licensed under GNU General Public License GPL v3 (http://www.gnu.org/licenses/gpl.html) */
#include <user.h>
Flux::insensitive_map<User *> UserNickList;
uint32_t usercnt = 0, maxusercnt = 0;
User::User(Network *net, const Flux::string &snick, const Flux::string &sident, const Flux::string &shost, const Flux::string &srealname, const Flux::string &sserver){
 /* check to see if a empty string was passed into the constructor */
 if(snick.empty() || sident.empty() || shost.empty())
   throw CoreException("Bad args sent to User constructor");
 
 this->nick = snick;
 this->n = net;
 this->ident = sident;
 this->host = shost;
 this->realname = srealname;
 this->server = sserver;
 this->fullhost = snick+"!"+sident+"@"+shost;
 UserNickList[snick] = this;
 Log(LOG_RAWIO) << "New user! " << this->nick << '!' << this->ident << '@' << this->host << (this->realname.empty()?"":" :"+this->realname);
 ++usercnt;
 if(usercnt > maxusercnt){
  maxusercnt = usercnt;
  Log(LOG_TERMINAL) << "New maximum user count: " << maxusercnt;
 }
}
User::~User(){
  Log() << "Deleting user " << this->nick << '!' << this->ident << '@' << this->host << (this->realname.empty()?"":" :"+this->realname);
  UserNickList.erase(this->nick);
}

void User::SendWho(){ this->n->ircproto->who(this->nick); }

void User::SendMessage(const char *fmt, ...){
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  this->SendMessage(Flux::string(buffer));
  va_end(args);
}

void User::SendPrivmsg(const char *fmt, ...){
 char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  this->SendPrivmsg(Flux::string(buffer));
  va_end(args); 
}

bool User::IsOwner(){
   return true; //FIXME: removve this.
}

void User::SetNewNick(const Flux::string &newnick)
{
  if(newnick.empty())
    throw CoreException("User::SetNewNick() was called with empty arguement");
  
  UserNickList.erase(this->nick);
  this->nick = newnick;
  UserNickList[this->nick] = this;
}

void User::AddChan(Channel *c){ if(c) ChannelList[c] = this; }
void User::DelChan(Channel *c)
{
  CList::iterator it = ChannelList.find(c);
  if(it != ChannelList.end())
    ChannelList.erase(it);
}

Channel *User::findchannel(const Flux::string &name)
{
  auto it1 = this->n->ChanMap.find(name);
  Channel *c = it1->second;
  if(!c)
    return NULL;
  CList::iterator it = ChannelList.find(c);
  if(it != ChannelList.end())
    return it->first;
  return NULL;
}

void User::SendMessage(const Flux::string &message){ this->n->ircproto->notice(this->nick, message); }
void User::SendPrivmsg(const Flux::string &message){ this->n->ircproto->privmsg(this->nick, message); }

User *FindUser(const Flux::string &fnick){
  auto it = UserNickList.find(fnick);
  if(it != UserNickList.end())
    return it->second;
  return NULL;
}