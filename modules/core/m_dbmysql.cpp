/* RequiredLibraries: mysqlclient */

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
#define NO_CLIENT_LONG_LONG
#include <mysql/mysql.h>

// FIXME: TODO: ugh this is such a mess, clean this up and make it actually work!

class MySQLInterface : public Base
{
 MYSQL *conn;
 public:
  MySQLInterface(const Flux::string &hostname, const Flux::string &username, const Flux::string &password, const Flux::string &dbname, int port = 0) : Base()
  {
    conn = mysql_init(NULL);
    
    if(hostname.empty() || username.empty() || password.empty() || dbname.empty())
      throw ModuleException("Empty parameter in MySQL Interface?");
    
    if(!mysql_real_connect(conn, hostname.c_str(), username.c_str(), password.c_str(), dbname.c_str(), port, NULL, 0))
      // Throw CoreException instead of module exception because the error should be dealt with.
      throw CoreException(printfify("Cannot connect to MySQL database: %s (%u)", mysql_error(conn), mysql_errno(conn)));
      
    Log(LOG_DEBUG) << "[MySQL] Successfully connected to " << hostname << ':' << port << " using database \"" << dbname << "\"";
  }
  
  MYSQL *GetConnection() const
  {
    return this->conn;
  }
  
  ~MySQLInterface()
  {
    if(conn) // Close the database connection
      mysql_close(conn);
    conn = nullptr;
  }
};

MySQLInterface *me;

// cppdb::session sql("mysql:database="+Config->dbname+";user="+Config->dbuser+";password='"+Config->dbpass+"'");
void Read(module *m = nullptr)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  int num_fields;
  Flux::vector params;
  
  // mysql_query(me->GetConnection(), ""); <-- do something here?
  result = mysql_store_result(conn);
  num_fields = mysql_num_fields(result);

  while ((row = mysql_fetch_row(result)))
  {
      for(int i = 0; i < num_fields; i++)
      {
          params.push_back(row[i]?row[i]:"NULL");
          //printf("%s ", row[i] ? row[i] : "NULL");
      }
  }

  mysql_free_result(result);
  
  if(m)
    m->OnDatabasesRead(params);
  else
  {
    FOREACH_MOD(I_OnDatabasesRead, OnDatabasesRead(params));
  }
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
    Log() << "[MySQL] Using MySQL client version " << mysql_get_client_info();
    me = new MySQLInterface(Config->something, Config->Something, Config->Somethingelse, Config->Notimplementedyet, Config->Portidontknow);
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