/* All code is licensed under GNU General Public License GPL v3 (http://www.gnu.org/licenses/gpl.html) */
#include "command.h"
#include "module.h"
/**
 *\file  command.cpp 
 *\brief Contains the command class.
 */
Oper::Oper(){}
void Oper::samode(const Flux::string &target, const Flux::string &mode){
  send_cmd("SAMODE %s %s\n", target.c_str(), mode.c_str());
}
void Oper::samode(const Flux::string &target, const Flux::string &mode, const Flux::string &params){
  send_cmd("SAMODE %s %s %s\n", target.c_str(), mode.c_str(), params.c_str());
}
void Oper::sajoin(const Flux::string &target, const Flux::string &channel){
  send_cmd("SAJOIN %s %s\n", target.c_str(), channel.c_str());
}
void Oper::sapart(const Flux::string &target, const Flux::string &channel){
  send_cmd("SAPART %s %s\n", target.c_str(), channel.c_str());
}
void Oper::sanick(const Flux::string &target, const Flux::string &nickname){
  send_cmd("SANICK %s %s\n", target.c_str(), nickname.c_str());
}
void Oper::sakick(const Flux::string &channel, const Flux::string &target, const Flux::string &reason){
  send_cmd("SAKICK %s %s %s\n", channel.c_str(), target.c_str(), reason.c_str());
}
void Oper::satopic(const Flux::string &target, const Flux::string &topic){
  send_cmd("SATOPIC %s %s\n", target.c_str(), topic.c_str());
}
void Oper::satopic(const Flux::string &target, const char *fmt, ...){
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  this->satopic(target, Flux::string(buffer));
  va_end(args); 
}
void Oper::sahost(const Flux::string &target, const Flux::string &host){
  send_cmd("CHGHOST %s %s\n", target.c_str(), host.c_str());
}
void Oper::saident(const Flux::string &target, const Flux::string &ident){
  send_cmd("CHGIDENT %s %s\n", target.c_str(), ident.c_str());
}
void Oper::kill(const Flux::string &target, const Flux::string &reason){
  send_cmd("KILL %s %s\n", target.c_str(), reason.c_str());
}
void Oper::saname(const Flux::string &target, const Flux::string &name){
  send_cmd("CHGNAME %s %s\n", target.c_str(), name.c_str());
}
void Oper::saname(const Flux::string &target, const char *fmt, ...){
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  this->saname(target, Flux::string(buffer));
  va_end(args); 
}
void Oper::wallops(const Flux::string &message){
  send_cmd("WALLOPS %s\n", message.c_str());
}
void Oper::wallops(const char *fmt, ...){
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  this->wallops(Flux::string(buffer));
  va_end(args); 
}
void Oper::globops(const Flux::string &message){
  send_cmd("GLOBOPS %s\n", message.c_str());
}
void Oper::globops(const char *fmt, ...){
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  this->globops(Flux::string(buffer));
  va_end(args); 
}
void Oper::zline(const Flux::string &ipmask, const Flux::string &time, const Flux::string &reason){
  send_cmd("ZLINE %s %s %s\n", ipmask.c_str(), time.c_str(), reason.c_str());
}
void Oper::qline(const Flux::string &target, const Flux::string &time, const Flux::string &reason){
  send_cmd("QLINE %s %s %s\n", target.c_str(), time.c_str(), reason.c_str());
}
void Oper::kline(const Flux::string &target, const Flux::string &time, const Flux::string &reason){
  send_cmd("KLINE %s %s %s\n", target.c_str(), time.c_str(), reason.c_str());
}
void Oper::gline(const Flux::string &target, const Flux::string &time, const Flux::string &reason){
  send_cmd("GLINE %s %s %s\n", target.c_str(), time.c_str(), reason.c_str());
}
/*******************************************************************************************/
/**
 * \brief This allows us to reply in a simple way to the user from the CommandSource struct
 * \fn void CommandSource::Reply(const char *fmt, ...)
 * \param char* The message in a c string format
 * \param va_list any other functions, vars to pass to va_list to form the string
 */
void CommandSource::Reply(const char *fmt, ...){
  va_list args;
  char buf[BUFSIZE];
  if(fmt){
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    this->Reply(Flux::string(buf));
    va_end(args);
  }
}
/** \overload void CommandSource::Reply(const Flux::string &msg) */
void CommandSource::Reply(const Flux::string &msg){
 sepstream sep(msg, '\n');
 Flux::string tok;
 while(sep.GetToken(tok))
   this->u->SendMessage(tok);
}

/**
 * \brief Find a command in the command map
 * \fn Command *FindCommand(const Flux::string &name)
 * \return Command class you wanted to find
 * \param name A string containing the command name you're looking for
 */
Command *FindCommand(const Flux::string &name, CommandType type){
  if(name.empty())
    return NULL;
  if((type == COMMAND_PRIVATE))
  {
    auto it = Commandsmap.find(name);
      if(it != Commandsmap.end())
	return it->second;
  }
  else if((type == COMMAND_CHANNEL))
  {
    auto it = ChanCommandMap.find(name);
    if(it != ChanCommandMap.end())
      return it->second;
  }
  return NULL;
}
/*******************************************************************************************/
/* why is this in here with the rest of the commands that send to the server? i dont fucking know lol */
/**
 * \class Command A class which most private message commands inside of modules work in.
 * \fn Command::Command(const Flux::string &sname, size_t min_params, size_t max_params)
 * \param Flux::string command name
 * \param size_t the minimum size of the buffer the command vector gets
 * \param size_t the maximum size the vector gets
 */
Command::Command(const Flux::string &sname, size_t min_params, size_t max_params): MaxParams(max_params), MinParams(min_params), name(sname)
{
  for(unsigned i=0; i < sname.size(); ++i) //commands with spaces can screw up the command handler
    if(isspace(sname[i]))
      throw ModuleException("Commands cannot contain spaces!");
  this->mod = NULL;
}
Command::~Command()
{
  if(this->mod){
    auto it = ChanCommandMap.find(this->name);
   if(it->second != NULL)
     this->mod->DelChanCommand(this);
   else
     this->mod->DelCommand(this);
  }
}
/**
 * \brief Sets the command description
 * \fn void Command::SetDesc(const Flux::string &d)
 * \param Flux::string The description
 */
void Command::SetDesc(const Flux::string &d){ this->desc = d; }
/**
 * \brief Sets the syntax of the command
 * \fn void Command::SetSyntax(const Flux::string &s)
 * \param Flux::string The syntax
 */
void Command::SetSyntax(const Flux::string &s){ this->syntax.push_back(s); }
/**
 * \brief Sends the syntax ONLY from the module, cannot be executed from outside the module
 * \fn void Command::SendSyntax(CommandSource &source)
 * \param CommandSource The source for which the command was executed with.
 */
void Command::SendSyntax(CommandSource &source)
{
 if(!this->syntax.empty()){
  source.Reply("Syntax: \2%s %s\2", this->name.c_str(), this->syntax[0].c_str());
  for(unsigned i=1, j = this->syntax.size(); i < j; ++i)
    source.Reply("        \002%s %s\002", this->name.c_str(), this->syntax[i].c_str());
 }else{
   source.Reply("Syntax: \2%s\2", this->name.c_str()); //We dont have syntax's on some commands
 }
}
/** \overload void Command::SendSyntax(CommandSource &source, const Flux::string &syn) */
void Command::SendSyntax(CommandSource &source, const Flux::string &syn){
  source.Reply("Syntax: \2%s %s\2", this->name.c_str(), syn.c_str());
  source.Reply("\002/msg %s HELP %s\002 for more information.", Config->BotNick.c_str(), this->name.c_str());
}
/**
 * \brief Returns a flux::string with the commands description
 * \fn const Flux::string &Command::GetDesc() const
 * \return Flux::string with the description
 */
const Flux::string &Command::GetDesc() const{
 return this->desc; 
}
bool Command::OnHelp(CommandSource &source, const Flux::string &subcommand) { return false; }
void Command::OnList(User *u) { }
/**
 * \brief The Run function is used for when you run the command from IRC, it contains what the actual command does
 * \fn void Command::Run(CommandSource&, const std::vector<Flux::string>&)
 * \param CommandSource The user source who executed the command
 * \param std::vector<Flux::string> A Flux::string vector of the command's buffer
 */
void Command::Run(CommandSource&, const std::vector<Flux::string>&) { }
/**
 * \brief Send the syntax when a syntax error happened, this is NOT like SendSyntax.
 * \fn void Command::OnSyntaxError(CommandSource &source, const Flux::string &subcommand)
 * \param CommandSource Source from which the command was executed from
 * \param Flux::string A string of any sub commands that the command may have.
 */
void Command::OnSyntaxError(CommandSource &source, const Flux::string &subcommand)
{
 this->SendSyntax(source);
 auto it = ChanCommandMap.find(this->name);
 if((it->second != NULL)){}else
    source.Reply("\002/msg %s HELP %s\002 for more information.", Config->BotNick.c_str(), source.command.c_str());
}