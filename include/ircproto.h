/* Arbitrary Navn Tool -- IRC Protocol Class Prototype
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
#ifndef IRCPROTO_H
#define IRCPROTO_H
#include "includes.h"
#include "command.h"

class Network;
class IRCProto
{
public:
  const Network *net;
  IRCProto(const Network*);
  void introduce_client(const Flux::string&, const Flux::string&, const Flux::string&);
  void privmsg(const Flux::string &where, const char *fmt, ...);
  void privmsg(const Flux::string &where, const Flux::string &msg);
  void notice(const Flux::string &where, const char *fmt, ...);
  void notice(const Flux::string &where, const Flux::string &msg);
  void action(const Flux::string &where, const char *fmt, ...);
  void action(const Flux::string &where, const Flux::string &msg);

  void quit(const char*, ...);
  void Raw(const char*, ...);
  void nick(const char*, ...);
  void invite(const Flux::string&, const Flux::string&);
  void version(const Flux::string &server = "");
  void stats(const Flux::string&, const Flux::string &server = "");
  void links(const Flux::string &remote = "", const Flux::string &mask = "");
  void time(const Flux::string &server = "");
  void admin(const Flux::string &server = "");
  void info(const Flux::string &server = "");
  void whowas(const Flux::string&, int count = 0, const Flux::string &server = "");
  void users(const Flux::string &server = "");
  void userhost(const Flux::string&);

  void kick(const Flux::string&, const Flux::string&, const char *fmt, ...);
  void topic(const Flux::string&, const char *fmt, ...);
  void part(const Flux::string&, const char *fmt, ...);
  void kick(const Flux::string&, const Flux::string&, const Flux::string&);
  void mode(const Flux::string&, const Flux::string&, const Flux::string&);
  void topic(const Flux::string&, const Flux::string&);
  void part(const Flux::string&, const Flux::string&);
  void oper(const Flux::string&, const Flux::string&);
  void mode(const Flux::string&, const Flux::string&);
  void who(const Flux::string&);
  void quit(const Flux::string&);
  void nick(const Flux::string&);
  void part(const Flux::string&);
  void join(const Flux::string&);
  void whois(const Flux::string&);
  void names(const Flux::string&);
  void away(const Flux::string&);
};

class GlobalProto
{
public:
  GlobalProto();
  void privmsg(const Flux::string &where, const char *fmt, ...);
  void privmsg(const Flux::string &where, const Flux::string &msg);
  void notice(const Flux::string &where, const char *fmt, ...);
  void notice(const Flux::string &where, const Flux::string &msg);
  void action(const Flux::string &where, const char *fmt, ...);
  void action(const Flux::string &where, const Flux::string &msg);
  void quit(const char*, ...);
  void kick(const Flux::string&, const Flux::string&, const char *fmt, ...);
  void topic(const Flux::string&, const char *fmt, ...);
  void part(const Flux::string&, const char *fmt, ...);
  void kick(const Flux::string&, const Flux::string&, const Flux::string&);
  void mode(const Flux::string&, const Flux::string&, const Flux::string&);
  void topic(const Flux::string&, const Flux::string&);
  void part(const Flux::string&, const Flux::string&);
  void oper(const Flux::string&, const Flux::string&);
  void mode(const Flux::string&, const Flux::string&);
  void user(const Flux::string&, const Flux::string&);
  void who(const Flux::string&);
  void quit(const Flux::string&);
  void nick(const Flux::string&);
  void part(const Flux::string&);
  void join(const Flux::string&);
  void whois(const Flux::string&);
  void names(const Flux::string&);
  void away(const Flux::string&);
};

#endif
