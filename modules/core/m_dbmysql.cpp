/* Arbitrary Navn Tool -- MySQL Database Sync Module
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
// Since we use CPPCMS as the frontend (which is required), we can use its features here as well :P
#include <cppdb/frontend.h>

// FIXME: TODO: ugh this is such a mess, clean this up and make it actually work!

cppdb::session sql("mysql:database="+Config->dbname+";user="+Config->dbuser+";password='"+Config->dbpass+"'");
void Read(module *m = nullptr)
{
  cppdb::result res = sql << "SELECT * from " << Config->dbname << cppdb::exec;
}

void Write(const char *fmt, ...)
{
  char buffer[BUFSIZE] = "";
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  sql << buffer << "\n";
  va_end(args);
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