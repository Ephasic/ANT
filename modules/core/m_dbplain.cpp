#include "flux_net_irc.hpp"

/* Database Functions */

std::stringstream db_buffer;
void Write(const Flux::string &buf)
{
  db_buffer << buf << "\n";
}

void Read(module *m = NULL)
{
  std::fstream db;
  db.open("ANT.db", std::ios_base::in);
  if(!db.is_open())
  {
    Log() << "Unable to open ANT.db for reading!";
    return;
  }
  Flux::string buf;
  while(std::getline(db, buf.std_str()))
  {
    if(buf.empty())
      continue;
    
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
    Implementation i[] = { I_OnDatabasesWrite, I_OnDatabasesRead, I_OnLoadDatabases, I_OnModuleLoad, I_OnSaveDatabases };
  }
  void OnDatabasesWrite(void (*Write)(const Flux::string&))
  {
    
  }

  void OnDatabasesRead(const Flux::vector &params)
  {
    
  }

  void OnLoadDatabases()
  {
    Read();
  }
  
  void OnModuleLoad(module *m)
  {
    Read(m);
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