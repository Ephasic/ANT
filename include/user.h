/* Arbitrary Navn Tool -- User Prototypes
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#pragma once
#ifndef USER_H
#define USER_H
#include "ircproto.h"
#include "channel.h"
/**
 * \class User "user.h" USER_H
 * \brief A Class for IRC Users
 * This class wraps around IRC Users and includes several useful commands as well as User information.
 */
class User : public Base
{
public:
    User(Network*, const Flux::string&, const Flux::string&, const Flux::string&, const Flux::string &realname = "", const Flux::string &server ="");
    virtual ~User();

    Flux::string nick;
    Flux::string host;
    Flux::string realname;
    Flux::string ident;
    Flux::string fullhost;
    Flux::string server;
    Network *n;

    // Functions
    void AddChan(Channel*);
    void DelChan(Channel*);
    Channel *findchannel(const Flux::string&);
    virtual void SetNewNick(const Flux::string&);
    void SendWho();
    virtual void SendMessage(const Flux::string&);
    virtual void SendMessage(const char*, ...);
    void SendPrivmsg(const Flux::string&);
    void SendPrivmsg(const char*, ...);
};
#endif