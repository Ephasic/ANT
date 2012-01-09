/* Arbitrary Navn Tool -- MySQL Database Sync Module
 * 
 * (C) 2011-2012 Flux-Net
 * Contact us at Dev@Flux-Net.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

#include "modules.h"
#ifdef HAVE_MYSQL_MYSQL_H
 #include <mysql/mysql.h>

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
    Read();
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
#else
  #pragma "You need mysql development libraries to compile this module!"
#endif