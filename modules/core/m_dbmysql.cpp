#include "flux_net_irc.hpp"

void Read(module *m = NULL)
{
  
}

void Write(const char *fmt, ...)
{
  /* NULL :D */
}


class modmysql : public module
{
public:
  modmysql(const Flux::string &Name):module(Name)
  {
    this->SetAuthor("Justasic");
    this->SetVersion(VERSION);
    Implementation i[] = { I_OnDatabasesWrite, I_OnDatabasesRead, I_OnModuleLoad, I_OnSaveDatabases, I_OnForceDatabasesRead };
    ModuleHandler::Attach(i, this, sizeof(i)/sizeof(Implementation));
  }
  void OnLoad()
  {
    Log() << "[MySQL] Loading Databases.";
    Read()
  }
  
  void OnDatabasesWrite(void (*Write)(const char*, ...))
  {
    
  }
  
  void OnDatabasesRead(const Flux::vector &params)
  {
    Log() << "[MySQL] Reading Databases.";
  }
  
  void OnModuleLoad(module *m)
  {
    if(m != this)
      Read(m);
  }
  
  void OnSaveDatabases()
  {
    Log() << "[MySQL] Saving Databases..." ;
    FOREACH_MOD(I_OnDatabasesWrite, OnDatabasesWrite(Write));
  }
  
  void OnForceDatabasesRead()
  {
    Log() << "[MySQL] Reading Databases.";
    Read();
  }
};

MODULE_HOOK(modmysql)