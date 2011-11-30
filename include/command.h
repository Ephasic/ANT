/* All code is licensed under GNU General Public License GPL v3 (http://www.gnu.org/licenses/gpl.html) */
#ifndef command_h
#define command_h
/**
 * \include includes.h
 */
#include "includes.h"
/**
 * \include user.h
 */
#include "user.h"
/**
 *\file  command.h
 *\brief Contains the class Command
 */

/**
 * \class IsoHost
 * \brief Wrapper for an irc host
 * This was written by Justasic to break up the parts of a messages host for easier use.
 */
class CoreExport IsoHost:Flux::string
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
struct CoreExport CommandSource
{
  User *u;
  Channel *c; /* Channel name, this will be replaced with channel class */
  Network *n;
  Flux::string command;
  Flux::string message;
  Flux::string raw;
  std::vector<Flux::string> params;
  
  void Reply(const char *fmt, ...);
  void Reply(const Flux::string&);
};

/**
 * \class Command
 * \brief A wrapper clas for Module commands
 * Contains methods and properties for handling/getting information from module commands.
 * \note Not to be confused with the Commands class.
 */
class CoreExport Command : public Base
{
  Flux::string desc;
  std::vector<Flux::string> syntax;
public:
  size_t MaxParams;
  size_t MinParams;
  Flux::string name;
  CommandType type;
  module *mod;
  Command(const Flux::string &sname, size_t min_params=0, size_t max_params=0);
  virtual ~Command();
protected:
  void SetDesc(const Flux::string&);
  void SetSyntax(const Flux::string&);
  void SendSyntax(CommandSource&);
  void SendSyntax(CommandSource&, const Flux::string&);
public:
  const Flux::string &GetDesc() const;
  virtual void Run(CommandSource&, const std::vector<Flux::string> &params);
  virtual bool OnHelp(CommandSource&, const Flux::string&);
  virtual void OnList(User *u);
  virtual void OnSyntaxError(CommandSource&, const Flux::string&);
};
#endif