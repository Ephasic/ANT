/* Arbitrary Navn Tool -- Prototype for Command Routines
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
#ifndef command_h
#define command_h
#include "extern.h"
#include "includes.h"
#include "user.h"

/**
 * \class IsoHost
 * \brief Wrapper for an irc host
 * This was written by Justasic to break up the parts of a messages host for easier use.
 */
class IsoHost
{
public:
  IsoHost(const Flux::string&);
  Flux::string raw;
  Flux::string nick;
  Flux::string host;
  Flux::string ident;
};

/**
 * \struct CommandSource "user.h" USER_H
 * \brief A wrapper class for the source of the Command
 * Contains information about where the command came from, as well as a Reply() function to quickly send a PrivMessage to the Command sender.
 */
struct CommandSource
{
  User *u;
  Channel *c; /* Channel name, this will be replaced with channel class */
  Network *n;
  Bot *b;
  Flux::string command;
  Flux::string raw;
  std::vector<Flux::string> params;

  void Reply(const char *fmt, ...);
  void Reply(const wchar_t *fmt, ...);
  void Reply(const Flux::string&);
};

/**
 * \class Command
 * \brief A wrapper class for Module commands
 * Contains methods and properties for handling/getting information from module commands.
 * \note Not to be confused with the Commands class.
 */
class Command : public Base
{
  Flux::string desc;
  std::vector<Flux::string> syntax;
  CommandType type;
public:
  size_t MaxParams;
  size_t MinParams;
  Flux::string name;
  Module *mod;
  Command(Module *m, const Flux::string &sname, CommandType t = C_NULL, size_t min_params=0, size_t max_params=0);
  virtual ~Command();
protected:
  void SetDesc(const Flux::string&);
  void SetSyntax(const Flux::string&);
  void SendSyntax(CommandSource&);
  void SendSyntax(CommandSource&, const Flux::string&);
public:
  const Flux::string &GetDesc() const;
  const CommandType GetType() const;
  virtual void Run(CommandSource&, const std::vector<Flux::string> &params);
  virtual bool OnHelp(CommandSource&, const Flux::string&);
  virtual void OnList(User *u);
  virtual void OnSyntaxError(CommandSource&, const Flux::string&);
};
#endif