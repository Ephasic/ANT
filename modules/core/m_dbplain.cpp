/* Arbitrary Navn Tool -- Plain Text File Database Module
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

// TODO: DEPRECATE THIS!!!

/* Database Functions */

std::stringstream db_buffer;
void Write(const char *fmt, ...)
{
    char buffer[BUFSIZE] = "";
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    db_buffer << buffer << "\n";
    va_end(args);
}

void Read(Module *m = nullptr)
{
  std::fstream db;
  db.open("ANT.db", std::ios_base::in);
  if(!db.is_open())
  {
    Log() << "[db_plain] Unable to open ANT.db for reading!";
    return;
  }
  Flux::string buf;
  int lineno = 0;
  while(std::getline(db, buf.std_str()))
  {
    lineno++;
    if(buf.empty())
      continue;

    if(lineno == 1 && !buf.search_ci("VER 1"))
    {
      if(Config->dbforce)
	Log() << "[db_plain] Could not determine database version! Reading anyway..";
      else
      {
	Log() << "[db_plain] Could not determine database version! Canceling read!";
	return;
      }
    }

    sepstream sep(buf, ' ');
    Flux::vector params;
    while(sep.GetToken(buf))
    {
      if(buf[0] == ':')
      {
	buf.erase(buf.begin());
	if (!buf.empty() && !sep.StreamEnd())
	  params.push_back(buf + " " + sep.GetRemaining());
	else if (!sep.StreamEnd())
	  params.push_back(sep.GetRemaining());
	else if (!buf.empty())
	  params.push_back(buf);
	break;
      }else
	params.push_back(buf);
    }
    if(m)
      m->OnDatabasesRead(params);
    else{
      FOREACH_MOD(I_OnDatabasesRead, OnDatabasesRead(params));
    }
  }
  db.close();
}

class dbplain : public Module
{
public:
  dbplain(const Flux::string &Name):Module(Name, MOD_DATABASE)
  {
    this->SetAuthor("Justasic");
    this->SetVersion(VERSION);
    Implementation i[] = { I_OnDatabasesWrite, I_OnDatabasesRead, I_OnModuleLoad, I_OnSaveDatabases, I_OnForceDatabasesRead };
    ModuleHandler::Attach(i, this, sizeof(i)/sizeof(Implementation));

    this->OnForceDatabasesRead();
  }

  void OnDatabasesWrite(void (*Write)(const char*, ...))
  {
    for(auto it : Networks) // Save networks
    {
      Network *n = it.second;
      Write("N %s %s %s", n->hostname.c_str(), n->port.c_str(), n->name.c_str());
      if(n->b)
	Write("NB %s %s %s", n->name.c_str(), n->b->nick.c_str(), n->b->ident.c_str());
      for(auto cit : n->ChanMap) // Save Channels in those networks
      {
	Channel *c = cit.second;
	Write("NC %s %s", n->name.c_str(), c->name.c_str());
      } //We don't put realnames in the database as its too hard to catch what the full name is
    }
  }

  void OnDatabasesRead(const Flux::vector &params)
  {
    Flux::string key = params[0];
    if(key.empty())
      return;

    if(key.equals_ci("N"))
    {
      Network *n = FindNetwork(params[1]);
      if(!n)
      {
	if(params.size() >= 4)
	{
	  // 			host 	port 	    net name
	  n = new Network(params[1], params[2], params[3]);
	}

	else
	  Log(LOG_DEBUG) << "[db_plain] Unable to read network line!";
      }

      if(n && !n->s)
      {
	new ReconnectTimer(0, n); // Connect to networks.
	//n->Connect(); // FIXME: Why can't I just call this?
	//SocketEngine::Process(true);
      }
    }

    if(key.equals_ci("NC"))
    {
      Network *n = FindNetwork(params[1]);
      if(!n)
	Log(LOG_DEBUG) << "[db_plain] Unable to find network " << params[1] << " for channel creation";
      else
	n->JoinChannel(params[2]);
    }
//     if(key.equals_ci("NB"))
//     {
//       Network *n = FindNetwork(params[1]);
//       if(n){
// 	new Bot(n, params[2], params[3], Config->Realname);
//       }
//     }
  }

  void OnModuleLoad(Module *m)
  {
    if(m != this)
      Read(m);
  }

  void OnForceDatabasesRead()
  {
    Log() << "[db_plain] Reading Database.";
    Read();
  }

  void OnSaveDatabases()
  {
    std::fstream db;
    db.open("ANT.db", std::ios_base::out | std::ios_base::trunc);
    if(!db.is_open())
    {
      Log() << "[db_plain] Cannot save database! ANT.db";
      return;
    }
    db << "VER 1" << std::endl;
    FOREACH_MOD(I_OnDatabasesWrite, OnDatabasesWrite(Write));
    db << db_buffer.str();
    db_buffer.str("");
    db.close();
  }
};

MODULE_HOOK(dbplain)
