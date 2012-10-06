/* Arbitrary Navn Tool -- User Routines.
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#include "user.h"
#include "bot.h"

std::map<User*, std::vector<Channel*>> CUserMap;
// std::map<Channel*, std::vector<User*>> UChanMap;
uint32_t usercnt = 0, maxusercnt = 0;

User::User(Network *net, const Flux::string &snick, const Flux::string &sident, const Flux::string &shost, const Flux::string &srealname, const Flux::string &sserver) : nick(snick), host(shost), realname(srealname), ident(sident), fullhost(snick+"!"+sident+"@"+shost), server(sserver), n(net)
{
    /* check to see if a empty string was passed into the constructor */
    if(snick.empty() || sident.empty() || shost.empty())
	throw CoreException("Bad args sent to User constructor");
    if(!net)
	throw CoreException("User created with no network??");


    this->n->UserNickList[snick] = this;
    CUserMap[this] = std::vector<Channel*>();

    Log(LOG_RAWIO) << "New user! " << this->nick << '!' << this->ident << '@' << this->host << (this->realname.empty()?"":" :"+this->realname);

    ++usercnt;
    if(usercnt > maxusercnt)
    {
	maxusercnt = usercnt;
	Log(LOG_TERMINAL) << "New maximum user count: " << maxusercnt;
    }
}

User::~User()
{
    Log() << "Deleting user " << this->nick << '!' << this->ident << '@' << this->host << (this->realname.empty()?"":" :"+this->realname);
    this->n->UserNickList.erase(this->nick);
    CUserMap.erase(this);
    for(auto it : UChanMap)
    {
	for(std::vector<User*>::iterator it2 = it.second.begin(), it2_end = it.second.end(); it2 != it2_end; ++it2)
	    if((*it2) == this)
		it.second.erase(it2);
    }
}

void User::SendWho()
{
    this->n->b->ircproto->who(this->nick);
}

void User::SendMessage(const char *fmt, ...)
{
    char buffer[BUFSIZE] = "";
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    this->SendMessage(Flux::string(buffer));
    va_end(args);
}

void User::SendPrivmsg(const char *fmt, ...)
{
    char buffer[BUFSIZE] = "";
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    this->SendPrivmsg(Flux::string(buffer));
    va_end(args);
}

void User::SetNewNick(const Flux::string &newnick)
{
    if(newnick.empty())
	throw CoreException("User::SetNewNick() was called with empty arguement");

    Log(LOG_TERMINAL) << "Setting new nickname: " << this->nick << " -> " << newnick;
    this->n->UserNickList.erase(this->nick);
    this->nick = newnick;
    this->n->UserNickList[this->nick] = this;
}

void User::AddChan(Channel *c)
{
    if(c)
	CUserMap[this].push_back(c);
}

void User::DelChan(Channel *c)
{
    if(c)
    {
	for(auto it : CUserMap)
	{
	    if(it.first == this)
		for(std::vector<Channel*>::iterator it2 = it.second.begin(), it2_end = it.second.end(); it2 != it2_end; ++it2)
		    if((*it2) == c)
			CUserMap[this].erase(it2);
	}
	if(CUserMap[this].empty())
	    delete this; // remove the user, it has no channels.
    }
}

Channel *User::findchannel(const Flux::string &name)
{
    for(auto it : CUserMap)
    {
	if(it.first == this)
	    for(auto it2 : it.second)
		if(it2->name.equals_ci(name))
		    return it2;
    }

    return nullptr;
}

void User::SendMessage(const Flux::string &message)
{
    this->n->b->ircproto->notice(this->nick, message);
}

void User::SendPrivmsg(const Flux::string &message)
{
    this->n->b->ircproto->privmsg(this->nick, message);
}