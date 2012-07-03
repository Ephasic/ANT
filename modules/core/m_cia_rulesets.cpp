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
#include "colors.h"

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
// 	Log(LOG_TERMINAL) << "DIR: " << dir;
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

Flux::map<Flux::vector> PosixCommonPrefix(const Flux::vector &files)
{
  Flux::map<Flux::vector> CommonDirectories;
  for(Flux::vector::const_iterator it = files.begin(); it != files.end(); ++it)
  {
    Flux::string path = *it;
    Flux::string directory = path.substr(0, path.rfind('/'));
    Flux::string file = path.substr(path.rfind('/') + 1);
    Log(LOG_TERMINAL) << "DIR: " << directory << " FILE: " << file;

    // Initialize the vector
    if(CommonDirectories.find(directory) == CommonDirectories.end())
      CommonDirectories[directory] = Flux::vector();

    CommonDirectories[directory].push_back(file);
  }
  
  return CommonDirectories;
}

Flux::vector ConsolidateFiles(const Flux::vector &files)
{
  if (files.size() <= 2)
    return files;

  // TODO: make this into (%s changed, %s files) or whatever
  return files;
}


/**************** Module Class *******************/

class CIA_RULESET_MOD : public Module
{
  CommitMessage Message;
public:
  CIA_RULESET_MOD(const Flux::string &Name):Module(Name, MOD_NORMAL)
  {
    this->SetAuthor("Justasic");
    this->SetVersion(VERSION);
    ModuleHandler::Attach(I_OnCommit, this);
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

  // Used to make string formatting less bulky
  Flux::string FormatString(const Flux::string &string)
  {
    static struct special_chars
    {
      Flux::string character;
      Flux::string replace;
      special_chars(const Flux::string &c, const Flux::string &r) : character(c), replace(r) { }
    }
    special[] = {
      special_chars("{project}", this->GetCommitData("project")),
      special_chars("{author}", this->GetCommitData("author")),
      special_chars("{branch}", this->GetCommitData("branch")),
      special_chars("{timestamp}", this->GetCommitData("timestamp")),
      special_chars("{module}", this->GetCommitData("module")),
      special_chars("{scriptname}", this->GetCommitData("scriptname")),
      special_chars("{scriptversion}", this->GetCommitData("scriptversion")),
      special_chars("{scripturl}", this->GetCommitData("scripturl")),
      special_chars("{scriptauthor}", this->GetCommitData("scriptauthor")),
      special_chars("{revision}", this->GetCommitData("revision")),
      special_chars("{insertions}", this->GetCommitData("insertions")),
      special_chars("{deletions}", this->GetCommitData("deletions")),
      special_chars("{log}", this->GetCommitData("log")),
      special_chars("","")
    };
    Flux::string ret = string;
    for(int i = 0; special[i].character.empty() == false; ++i)
      ret = ret.replace_all_ci(special[i].character, special[i].replace);
    return ret.c_str();
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

    Flux::vector Files = msg.Files;
    Flux::map<Flux::vector> paths = PosixCommonPrefix(Files);

    for(Flux::map<Flux::vector>::iterator it = paths.begin(); it != paths.end(); ++it)
    {
      Log(LOG_TERMINAL) << "Directory \"" << it->first << "\":";
      Flux::vector difiles = it->second;
      for(unsigned i = 0; i < difiles.size(); ++i)
	Log(LOG_TERMINAL) << "  " << difiles[i];
    }

    Flux::string files = BuildFileString(msg.Files);

    for(auto it : msg.Channels)
    {
      Channel *c = it;
      Log(LOG_DEBUG) << "Announcing in " << c->name << " (" << c->n->name << ')';

      // Build the commit message with stringstream
      // TODO: Make this a formatted string and interpret that.
      Flux::string message =
      printfify("\0034\002{project}: \017\0037{author} * \017\002[{branch}]\017\0038 r{revision}"
		"\017 ~\0036 {insertions}(+) {deletions}(-)\017\002 | \017\00310%s\017: {log}", files.c_str());

      Flux::string formattedmessgae = FormatString(message);

      //Log(LOG_TERMINAL) << "Commit Msg: \"" <<  formattedmessgae << "\"";
      c->SendMessage(formattedmessgae);
    }
  }
};

MODULE_HOOK(CIA_RULESET_MOD)
