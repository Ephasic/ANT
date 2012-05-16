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

// For throws
class SQLException : public CoreException
{
public:
  SQLException(const Flux::string &message) : CoreException(message, "MySQL") { }
  virtual ~SQLException() throw() { }
};

class MySQLResult : public Base
{
public:
  MySQLResult(unsigned i) // needs finishing
  {
    
  }
  
  ~MySQLResult()
  {
    
  }
};


class MySQLInterface : public Base
{
  MYSQL *conn;
  Flux::string hostname;
  Flux::string username;
  Flux::string password;
  Flux::string database;
  int port;
public:
  MySQLInterface(const Flux::string &host, const Flux::string &user, const Flux::string &pass, const Flux::string &dbname, int p = 0) : Base(), hostname(host), username(user), password(pass), database(dbname)
  {
    if(hostname.empty() || username.empty() || password.empty() || database.empty())
      throw SQLException("Empty parameter in MySQL Interface?");

    this->Connect();
  }

  void Connect()
  {
    this->conn = mysql_init(NULL);

    if(!mysql_real_connect(conn, hostname.c_str(), username.c_str(), password.c_str(), database.c_str(), port, NULL, 0))
      throw SQLException(printfify("Cannot connect to MySQL database: %s (%u)", mysql_error(conn), mysql_errno(conn)));

    Log(LOG_DEBUG) << "[MySQL] Successfully connected to " << hostname << ':' << port << " using database \"" << database << "\"";
  }

  bool CheckConnection()
  {
    if(!this->conn || mysql_ping(this->conn))
    {
      try
      {
	this->Connect();
      }
      catch(const SQLException &e)
      {
	Log() << "[MySQL] " << e.GetReason();
	return false;
      }
    }

    return true;
  }

  Flux::string Escape(const Flux::string &query)
  {
    char buffer[BUFSIZE];
    mysql_real_escape_string(this->conn, buffer, query.c_str(), query.length());
    return buffer;
  }

  MYSQL_RES *RunQuery(const Flux::string &query)
  {
    mysql_query(this->conn, query.c_str());
    return mysql_store_result(this->conn);
  }
  
  const MYSQL *GetConnection() const
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

void Read(module *m = nullptr)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  int num_fields;
  Flux::vector params;
  
//   result = me->RunQuery("");
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
//   sql << buffer << "\n";
  va_end(args);
  Flux::vector params = ParametizeString(buffer, ' ');
  
  if(params.size() > 0)
  {
    Flux::string key = params[1];

    if(key.equals_ci("N"))
    {
      me->RunQuery("");
    }
    else if(key.equals_ci("NC"))
    {
      me->RunQuery("");
    }
    else if(key.equals_ci("NB"))
    {
      me->RunQuery("");
    }
  }
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
    me = new MySQLInterface(Config->sqlhost, Config->sqluser, Config->sqlpass, Config->sqldb, Config->sqlport);
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