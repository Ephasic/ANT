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

typedef Flux::map<Flux::string> RowMap;

struct QueryData
{
  Flux::string query;
  bool escape;
}

class MySQLResult : public Base
{
  MYSQL_RES *res;
  std::vector<RowMap> entries;
public:
  unsigned int id;
  Flux::string FinishedQuery;
  
  MySQLResult(unsigned i, const Flux::string &q, MYSQL_RES *r): id(i), res(r), FinishedQuery(q);
  {
    unsigned num_fields = res?mysql_num_fields(res):0;

    if(!num_fields)
      return;

    for(MYSQL_ROW row; (row = mysql_fetch_row(res));)
    {
      MYSQL_FIELD *fields = mysql_fetch_fields(res);

      if(fields)
      {
	RowMap items;

	for(unsigned field_count = 0; field_count < num_fields; ++field_count)
	{
	  Flux::string column = (fields[field_count].name?fields[field_count].name:"");
	  Flux::string data = (row[field_count]?row[field_count]:"");

	  items[column] = data;
	}

	this->entries.push_back(items);
      }
    }
  }

  inline const unsigned int GetID() const { return this->id; }

  size_t Rows() const { return this->entries.size(); }

  const RowMap &Row(size_t index) const
  {
    try
    {
      return this->entries[index];
    }
    catch (const std::out_of_range &)
    {
      throw SQLException("Out of bounds access of MySQLResult");
    }
  }

  const Flux::string Get(size_t index, const Flux::string &col) const
  {
    const RowMap rows = this->Row(index);

    RowMap::const_iterator it = rows.find(col);
    if(it == rows.end())
      throw SQLException("Unknown column name: " + col);

    return it->second;
  }
  
  ~MySQLResult()
  {
    if(this->res)
      mysql_free_result(this->res);
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

  Flux::string BuildQuery()
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

  MySQLResult RunQuery(const Flux::string &query)
  {
    if(this->CheckConnection() && !mysql_real_query(this->conn, real_query.c_str(), real_query.length()))
    {
      MYSQL_RES *res = mysq_store_result(this->conn);
      unsigned id = mysq_insert_id(this->conn);

      return MySQLResult(id, real_query, res);
    }
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

static MySQLInterface *me;

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
  Flux::vector params = ParamitizeString(buffer, ' ');
  
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
    try
    {
      me = new MySQLInterface(Config->sqlhost, Config->sqluser, Config->sqlpass, Config->sqldb, Config->sqlport);
      Read();
    }
    catch (const SQLException &e)
    {
      Log() << "Unable to initialize MySQL Interface: " << e.GetReason();
      throw;
    }
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