/* Arbitrary Navn Tool -- CIA style ruleset parser
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
#define NORMAL "\17"
#define BOLD "\2"
#define REVERSE ""
#define UNDERLINE "\13"

// TODO: This needs to parse CIA style rulesets and then do a FOREACH_MOD(I_OnCommit, OnCommit) call
// to announce the messages in all of IRC and other systems. This should also be one of the main calls
// for all announcements in this system.


// Calculate files to announce.
// FIXME: This needs to calculate directories as well
// FIXME: This needs to be handled by the Rulesets system later on.
// link to CIA file formatter: http://code.google.com/p/cia-vc/source/browse/trunk/cia/LibCIA/Formatters/Commit.py

// FIXME: This needs some serious fixing! It should calculate directories and files the same as CIA.vc
Flux::string BuildFileString(Flux::vector files)
{
  Flux::string ret;
  if(files.size() <= 3)
  {
    for(auto it : files)
    {
      Flux::string file = it;
      size_t slash = file.rfind("/");
      if(slash < file.size())
      {
	Log(LOG_TERMINAL) << "File: " << file << " slash: " << slash;
	Flux::string f = file.substr(slash+1);
	ret += f + " ";
      }
      else
	ret += file + " ";
    }
  }
  else
  {
    int dirs = 0;
    for(auto it : files)
    {
      Flux::string file = it;
      size_t slash = file.rfind("/");
      
      if(slash < file.size())
      {
	Flux::string dir = file.substr(0, slash);
	Log(LOG_TERMINAL) << "DIR: " << dir;
	if(!dir.empty())
	  dirs++;
      }
    }
    
    std::stringstream ss;
    ss << "(" << files.size() << " files";
    
    if(dirs < 0)
      ss << " in " << dirs;
    
    ss << " changed)";
    //ret = "(" + value_cast<Flux::string>(files.size()) + " files" + (dirs < 0?"":" in "+dirs) + " changed)";
    ret = ss.str();
  }
  ret.trim();
  return ret;
}

/**************** Throttling ******************/

// Someone just please clean this up?
std::queue<std::pair<Flux::string, CommitMessage>> throttledmessages;
class ThrottleTimer : public Timer
{
  void CleanQueue()
  {
    while(!throttledmessages.empty() && ++throttlecount <= 5)
    {
      std::pair<Flux::string, CommitMessage> msgdata = throttledmessages.front();
      throttledmessages.pop();
      
      for(auto it : msgdata.second.Channels)
	it->SendMessage(msgdata.first);
    }
  }
public:
  int throttlecount;
  ThrottleTimer():Timer(5), throttlecount(0)
  {
    // something here?
  }
  
  void Tick(time_t)
  {
    this->throttlecount = 0;
    this->CleanQueue();
    Log(LOG_TERMINAL) << "Throttle reset!";
  }
};

/**************** Module Class *******************/

class CIA_RULESET_MOD : public module
{
  CommitMessage Message;
public:
  CIA_RULESET_MOD(const Flux::string &Name):module(Name)
  {
    this->SetAuthor("Justasic");
    this->SetVersion(VERSION);
    Implementation i[] = { I_OnCommit };
    ModuleHandler::Attach(i, this, sizeof(i)/sizeof(Implementation));
  }

private:
  // Small api for getting commit data easily
  Flux::string GetCommitData(const Flux::string &n)
  {
    CommitMessage msg = this->Message;
    for(auto it : msg.info)
    {
      if(it.first.equals_ci(n))
	return it.second;
    }
    return "";
  }

public:
  
  void OnCommit(CommitMessage &msg)
  {
    EventResult e;
    FOREACH_RESULT(I_OnPreCommit, OnPreCommit(msg), e);
    if(e == EVENT_STOP)
    {
      Log(LOG_DEBUG) << "Module canceled commit message!";
      return;
    }
    
    this->Message = msg;
    // FIXME: if they're no connections, buffer the message
    Log(LOG_DEBUG) << "AnnounceCommit Called.";
    
    Flux::string files = BuildFileString(msg.Files);
    
    for(auto it : msg.Channels)
    {
      Channel *c = it;
      Log(LOG_DEBUG) << "Announcing in " << c->name << " (" << c->n->name << ')';
      
      // Build the commit message with stringstream
      std::stringstream ss;
      ss << RED << BOLD << this->GetCommitData("project") << ": " << NORMAL << ORANGE << this->GetCommitData("author") << " * ";
      ss << NORMAL << BOLD << '[' << this->GetCommitData("branch") << "] " << NORMAL << YELLOW << 'r'
      << this->GetCommitData("revision");
      ss << NORMAL << BOLD << " | " << NORMAL << AQUA << files << NORMAL << ": " << this->GetCommitData("log"); //<< files;
      
      Flux::string formattedmessgae = Flux::string(ss.str()).strip('\"').strip();
      
      //Log(LOG_TERMINAL) << "Commit Msg: \"" <<  formattedmessgae << "\"";
      ThrottleTimer *tt = new ThrottleTimer();
      if(++tt->throttlecount <= 5)
	c->SendMessage(formattedmessgae);
      else
	throttledmessages.push(std::make_pair(formattedmessgae, msg));
    }
  }
};

MODULE_HOOK(CIA_RULESET_MOD)