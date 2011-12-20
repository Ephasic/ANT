#include "flux_net_irc.hpp"

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

void Read(module *m = NULL)
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
	Log() << "[db_plain] Could not determin database version! Reading anyways..";
      else
      {
	Log() << "[db_plain] Could not determin database version! Canceling read!";
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

class dbplain : public module
{
public:
  dbplain(const Flux::string &Name):module(Name)
  {
    this->SetAuthor("Justasic");
    this->SetVersion(VERSION);
    Implementation i[] = { I_OnDatabasesWrite, I_OnDatabasesRead, I_OnModuleLoad, I_OnSaveDatabases, I_OnForceDatabasesRead };
  }
  void OnDatabasesWrite(void (*Write)(const char*, ...))
  {
    for(auto it : Networks) // Save networks
    {
      Network *n = it.second;
      Write("N %s %s %s", n->name.c_str(), n->hostname.c_str(), n->port.c_str());
      for(auto it : n->ChanMap) // Save Channels in those networks
      {
	Channel *c = it.second;
	Write("NC %s %s", n->name.c_str(), c->name.c_str());
      }
    }
//     for(auto it : Bots)
//     {
//       Bot *b = it.second;
//       Write("B %s %s %s", b->nick.c_str(), b->ident.c_str(), b->realname.c_str());
//     }
  }

  void OnDatabasesRead(const Flux::vector &params)
  {
    Flux::string key = params[0];
    if(key.equals_ci("N"))
    {
      new Network(params[1], params[2], params[3]);
    }
    if(key.equals_ci("NC"))
    {
      Network *n = FindNetwork(params[1]);
      if(!n)
	Log() << "[db_plain] Unable to find network " << params[1] << " for channel creation";
      else
	n->JoinChannel(params[2]);
    }
  }
  
  void OnModuleLoad(module *m)
  {
    if(m != this)
      Read(m);
  }

  void OnForceDatabasesRead()
  {
    Log() << "[db_plain] Reading Database.";
    Read();
  }
  
  void OnLoad()
  {
    Log() << "[db_plain] Loading Database.";
    Read(); // Load the databases
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