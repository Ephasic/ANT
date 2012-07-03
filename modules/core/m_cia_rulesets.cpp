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

Flux::vector PosixCommonPrefix(Flux::vector &files)
{
  Flux::map<int> endings;
  Flux::map<Flux::vector> endings2;
  Flux::vector dirs;

  for(Flux::vector::iterator it = files.begin(); it != files.end(); ++it)
  {
    Flux::string path = *it, dir, file;
    Flux::string::size_type pos = path.rfind("/") + 1;
    if(pos == Flux::string::npos)
      continue;

    file = path.substr(pos);
    dir = path.erase(pos);
    dirs.push_back(dir);

    *it = file;

    Log(LOG_TERMINAL) << "DIR: " << dir << " file: " << file << " IT: " << *it;

    bool found = false;
    for(Flux::map<int>::iterator it2 = endings.begin(); it2 != endings.end(); ++it2)
    {
      if(it2->first.equals_cs(dir))
      {
	it2->second++;
	found = true;
      }
    }

    for(Flux::map<Flux::vector>::iterator it2 = endings2.begin(); it2 != endings2.end(); ++it2)
    {
      if(it2->first.equals_cs(dir))
      {
	static_cast<void>(0);
// 	it2->second;
// 	found = true;
      }
    }

    if(!found)
    {
      endings[dir] = 1;
//       endings[dir] = 
    }

  }

  for(auto it : endings)
  {
    Log(LOG_TERMINAL) << "IT 1: " << it.first << " IT 2: " << it.second;
  }

  return dirs;
}

Flux::vector ConsolidateFiles(const Flux::vector &files)
{
  if (files.size() <= 1)
    return files;
  return files;
}

Flux::string CreateFileString(const Flux::vector &files)
{
  return "";
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
    PosixCommonPrefix(Files);

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
