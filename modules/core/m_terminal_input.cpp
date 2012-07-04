/* Arbitrary Navn Tool -- Basic Terminal Input Module
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

#include "modules.h"

// Fake user so we can use any command loaded into ANT
class CliUser : public User
{
  inline bool IsEven(unsigned int num) { return !(num & 1); }
  
  // Correct color codes when sending to the terminal
  Flux::string FixColors(const Flux::string &string)
  {
    Flux::string actualstring = string;
    unsigned found_bold = 0;
    unsigned found_under = 0;
    for(unsigned i = 0; i < actualstring.size(); ++i)
    {
      char c = actualstring[i];
      
      if(c == '\002')
      {
	i++;
	if(IsEven(++found_bold))
	  actualstring.insert(i, "\033[0m"+Config->LogColor);
	else
	  actualstring.insert(i, "\033[1m");
      }
      
      if(c == '\037')
      {
	i++;
	if(IsEven(++found_under))
	  actualstring.insert(i, "\033[0m"+Config->LogColor);
	else
	  actualstring.insert(i, "\033[4m");
      }
    }
    return Flux::Sanitize(actualstring);
  }
  
public:
  CliUser() : User(Networks.begin()->second, "Console", "Konsole", "magic.location", "Touch my body, feel me on the floor", "127.0.0.1")
  {
    // am i forgetting something here?
  }
  
  // forward messages to the console, not IRC
  void SendMessage(const Flux::string &message)
  {
    Log(LOG_TERMINAL) << FixColors(message);
  }
};

static CliUser *DeathBlade;

// Parse the command entered.
void ProcessInput(const Flux::string &str)
{
  if(DeathBlade)
  {
    Log(LOG_TERMINAL) << "Cannot process command, must be connected to a network!";
    return;
  }
  Flux::string Justasic = str;
  Justasic.trim();
  Flux::vector Lordofsraam = ParamitizeString(Justasic, ' ');
  Flux::string command = Lordofsraam.size() > 0 ? Lordofsraam[0] : "";
  command.trim();
  
  if(command[0] == '!')
  {
    DeathBlade->SendMessage("Commands cannot be prefixed with !");
    return;
  }
  
  CommandSource source;
  source.u = DeathBlade;
  source.c = nullptr;
  source.b = DeathBlade->n->b;
  source.n = DeathBlade->n;
  source.params = Lordofsraam;
  source.raw = Justasic;
  
  // The core can handle it now!
  ProcessCommand(source, Lordofsraam, "Console", "PRIVMSG");
}

/** \class InputThread
 * This thread allows for user input to be possible, this is activated when the nofork option is specified.
 */
class InputThread : public Thread
{
public:
  bool exiting;
  InputThread():Thread(), exiting(false)
  {
    Log(LOG_THREAD) << "Input Thread Initializing.";
    this->Start();
  }

  ~InputThread()
  {
    Log(LOG_THREAD) << "Input Thread Exiting.";
    exiting = true;
  }

  void ToRun()
  {
    base_string buf;
    while(!exiting)
    {
      Log(LOG_THREAD) << "Top of Input Loop";
      std::getline(std::cin, buf);
      if(!buf.empty())
	ProcessInput(buf);
    }
    SetExitState();
  }
};

class ModTerminalInput : public Module
{
  Thread *t;
public:
  CliUser *Justasic;
  ModTerminalInput(const Flux::string &Name):Module(Name, MOD_NORMAL), t(nullptr), Justasic()
  {
    this->SetVersion(VERSION);
    this->SetAuthor("Justasic");
    if(nofork && InTerm())
    {
      if(!t)
	t = new InputThread();
    }
    else
      throw ModuleException("Cannot run m_terminal_input when fork'ed");
    DeathBlade = this->Justasic;
  }
  
  ~ModTerminalInput()
  {
    if(t)
      delete t;
    if(Justasic)
      delete Justasic;
  }
};

MODULE_HOOK(ModTerminalInput)