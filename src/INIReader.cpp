/* Arbitrary Navn Tool -- Configuration Parser.
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

// Read an INI file into easy-to-access name/value pairs.

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include "INIReader.h"
#include "log.h"

// bool in_comment = false;
// bool in_quote = false;
// Flux::string quoted_text;

// void INIReader::Parse(const Flux::string &filename)
// {
//   SET_SEGV_LOCATION();
//   std::ifstream file(filename.c_str());
//   int linenum = 0;
//   Flux::string line, section, name, value;
//   if(file.is_open())
//   {
//     while(file.good())
//     {
//       bool contin = false;
//       std::getline(file, line.std_str());
//       linenum++;
//       line.trim();
//       //printf("UNPARSED: %s\n", line.c_str());
//
//       if(line[0] == '#' || line.empty())
// 	continue;
//
//       /********************************************/
//       unsigned c=0, len = line.length();
//       for(; c < len; ++c)
//       {
// 	char ch = line[c];
//
// 	if(in_quote)
// 	{
// 	  if(ch == '"')
// 	  {
// 	    in_quote = false;
// 	    Log(LOG_TERMINAL) << "TEXT: " << quoted_text;
// 	  }
// 	  else
// 	  {
// 	    Log(LOG_TERMINAL) << "INSIDE QUOTE: " << ch;
// 	    quoted_text += ch;
// 	  }
// 	  continue;
// 	}
// 	else if(ch == '"')
// 	{
// 	  in_quote = true;
// 	  quoted_text.clear();
// 	  continue;
// 	}
//
// 	if(in_comment)
// 	{
// 	  if(ch == '*' && c+1 < len && line[c+1] == '/')
// 	  {
// 	    in_comment = false;
// 	    line = line.erase(c, c+2);
// 	    line.trim();
// 	    if((line[0] == '[' && line[line.size() -1] == ']') || (!line.empty() && line.find_first_of('=')))
// 	      continue;
// 	    else
// 	      contin = true;
// 	    ++c;
// 	  }
// 	  continue;
// 	}
// 	else if(ch == '/' && c+1 < len && line[c+1] == '*')
// 	{
// 	  in_comment = true;
// 	  ++c;
// 	  continue;
// 	}
//       }
//
//       if(line.search("/*") && line.search("*/"))
//       {
// 	in_comment = contin = false;
// 	line = line.erase(line.find("/*"), line.find("*/"));
// 	line.trim();
//       }
//
//       if(in_comment || contin)
// 	continue;
//
//       /********************************************/
//       if(line[0] == '[' && line[line.size() -1] == ']')
//       {
// 	line = line.erase(0,1);
// 	section = line.erase(line.size()-1,line.size());
// 	section.trim();
//       }
//       else if((line[0] == '[' && line[line.size()-1] != ']') || (line[0] != '[' && line[line.size() -1] == ']'))
// 	throw ConfigException(printfify("Unterminated Bracket: %i", linenum));
//       else if(!line.empty() && line.find_first_of('='))
//       {
// 	name = line;
// 	int d = line.find_first_of('=');
//
// 	if(line.find_first_of(';') < static_cast<unsigned>(d))
// 	  throw ConfigException(printfify("Cannot have semi-colon immediately after assignment: %i", linenum));
// 	else if(d > 0)
// 	{
// 	  name = name.erase(d, name.size()-d);
// 	  name.trim();
// 	}
// 	/************************************/
// 	value = line;
// 	value = value.erase(0,value.find('=')+1);
// 	value.trim();
// 	if(value.find_first_of(';'))
// 	{ //We only erase ';' (semi-colons) if we find them, we cannot erase # signs for
// 	  int i = value.find_first_of(';'); // channels would look like comments.. maybe we can fix this one day..
// 	  if(i > 0)
// 	  {
// 	    if(i+1 <= static_cast<int>(value.size()) && value[i+1] == ';')
// 	      value = value.replace_all_cs(";;", ";");
// 	    else
// 	      value = value.erase(i, Flux::string::npos);
// 	  }
// 	}
// 	value.trim();
// 	/************************************/
// 	if(value.empty() || section.empty() || name.empty())
// 	  throw ConfigException(printfify("Empty value/section/name: %i", linenum));
// 	else
// 	{
// 	  if(name.equals_ci("Module"))
// 	    modules.push_back(value);
// 	  else
// 	    _values[this->MakeKey(section, name)] = value;
// 	}
//       }
//       else
// 	throw ConfigException(printfify("Undefined data: %i", linenum));
//     }
//
//     if(in_comment)
//       throw ConfigException(printfify("Unterminated comment: %i", linenum));
//
//     if(in_quote)
//       throw ConfigException(printfify("Unterminated quote: %i", linenum));
//
//     file.close();
//   }
//   else
//     return -1;
//   return 0;
// }

void INIReader::Parse(const Flux::string &filename)
{
  std::ifstream file(filename.c_str());
  if(!file.is_open())
    throw ConfigException("Cannot open bot.conf");

  bool in_comment = false;
  bool in_quote = false;
  int lineno = 0;
  Flux::string section;

  while(file.good())
  {
    // Different things needed for the map.
    Flux::string key;
    Flux::string value;

    // Get the line from the file
    Flux::string line;
    std::getline(file, line.std_str());
    line.trim();
    unsigned len = line.length();
    lineno++;

    // Ignore pound comments and empty lines
    if(line[0] == '#' || line.empty())
      continue;
    if(line[0] == '/' && line[1] == '/')
      continue;

    // Get INI sections
    if((line[0] == '[' && line[len - 1] != ']') || (line[0] != '[' && line[len - 1] == ']'))
      throw ConfigException(printfify("Invalid or unterminated section at %i in configuration", lineno));

    if(line[0] == '[' && line[len - 1] == ']')
    {
      section = line;
      section.erase(0, 1);
      section.erase(section.length() - 1);
//       Log(LOG_TERMINAL) << "Section: " << section;
      continue;
    }

    if(line.search("//"))
      line = line.substr(line.find("//"));


    bool passedequals = false;
    // Parse the rest of the config char by char
    for(unsigned i = 0; i < len; ++i)
    {
      char ch = line[i];

      if(ch == '/' && i+1 < len && line[i+1] == '*')
      {
	in_comment = true;
// 	Log(LOG_TERMINAL) << "Found Comment line: " << ch;
	continue;
      }

      if(in_comment)
      {
	if(ch == '*' && i+1 < len && line[i+1] == '/')
	  in_comment = false;
// 	Log(LOG_TERMINAL) << (in_comment?"":"End ") << "comment line: " << ch;
	continue;
      }

      if(ch == '=')
      {
	key = line.substr(0, i);
	key.trim();
// 	Log(LOG_TERMINAL) << "KEY: " << key;
	passedequals = true;
      }

      if(isdigit(ch) && passedequals)
      {
	if(in_quote || in_comment)
	{
	  // what the fuck c++?
	}
	else
	{
// 	  Log(LOG_TERMINAL) << "DIGIT: " << ch;
	  value += ch;
	}
      }

      if(ch == '"' && !in_quote)
      {
// 	Log(LOG_TERMINAL) << "START QUOTE: " << ch;
	in_quote = true;
	continue;
      }

      if(in_quote)
      {
	if(ch == '"')
	{
// 	  Log(LOG_TERMINAL) << "END QUOTE: " << ch;
	  in_quote = false;
	  continue;
	}
// 	Log(LOG_TERMINAL) << "IN QUOTE: " << ch;
	value += ch;
      }

    } // for

    if(value.empty())
      continue;

//       Log(LOG_TERMINAL) << "";
//       Log(LOG_TERMINAL) << "SECTION: " << section;
//       Log(LOG_TERMINAL) << "VALUE: " << value;
//       Log(LOG_TERMINAL) << "KEY: " << key;
      Log(LOG_TERMINAL) << "TOTAL: " << this->MakeKey(section, key) << ": " << value;
//       Log(LOG_TERMINAL) << "";

    if(section.equals_ci("Modules") && key.equals_ci("Module"))
      this->Modules.push_back(value);
    else
      _values[this->MakeKey(section, key)] = value;

    } // while

    if(in_comment || in_quote)
    {
//       if(in_comment)
// 	Log(LOG_TERMINAL) << "Unterminated comment!";
//
//       if(in_quote)
// 	Log(LOG_TERMINAL) << "Unterminated quote!";
      throw ConfigException("Unterminated comment or quote");
    }
    file.close();

} // function

/**
 * \class INIReader The config parser class, this parses the INI file for config values
 * \brief This class contains the actual parsing of values from the text file to a map containing all the config values
 * \fn INIReader::INIReader(const Flux::string &filename)
 * \param Flux::string The filename to parse for a config value.
 */
INIReader::INIReader(const Flux::string &filename)
{
  this->Parse(filename);
}
INIReader::~INIReader() { }
/**
 * \brief Returns a Flux::string from the config containing whatever value you requested
 * \fn Flux::string INIReader::Get(const Flux::string &section, const Flux::string &name, const Flux::string &default_value)
 * \return Flux::string with your value from the config
 * \param Flux::string The section the value is in
 * \param Flux::string the name of the variable in the config
 * \param bool the default value if there is no value in the config
 */
Flux::string INIReader::Get(const Flux::string &section, const Flux::string &name, const Flux::string &default_value)
{
    Flux::string key = MakeKey(section, name);
    return _values.count(key) ? _values[key] : default_value;
}
/**
 * \brief Returns an boolean value from the config
 * \fn bool INIReader::GetBoolean(const Flux::string &section, const Flux::string &name, bool default_value)
 * \return boolean of the value
 * \param Flux::string The section the value is in
 * \param Flux::string the name of the variable in the config
 * \param bool the default value if there is no value in the config
 */
bool INIReader::GetBoolean(const Flux::string &section, const Flux::string &name, bool default_value){
  Flux::string valstr = Get(section, name, "");
  valstr.trim();
  if(valstr.empty())
    return default_value;
  if(valstr.equals_ci("yes") || valstr.equals_ci("1") || valstr.equals_ci("y") || valstr.equals_ci("true"))
    return true;
  else if(valstr.equals_ci("false") || valstr.equals_ci("0") || valstr.equals_ci("n") || valstr.equals_ci("no"))
    return false;
  return default_value;
}
/**
 * \brief Returns an integer value from the config
 * \fn long INIReader::GetInteger(const Flux::string &section, const Flux::string &name, long default_value)
 * \return long integer of the value
 * \param Flux::string The section the value is in
 * \param Flux::string the name of the variable in the config
 * \param long the default value if there is no value in the config
 */
long INIReader::GetInteger(const Flux::string &section, const Flux::string &name, long default_value)
{
    Flux::string valstr = Get(section, name, "");
    const char* value = valstr.c_str();
    char* end;
    // This parses "1234" (decimal) and also "0x4D2" (hex)
    long n = strtol(value, &end, 0);
    return end > value ? n : default_value;
}

Flux::string INIReader::MakeKey(const Flux::string &section, const Flux::string &name)
{
    Flux::string key = section + "." + name;
    // Convert to lower case to make lookups case-insensitive
    for (unsigned i = 0; i < key.length(); i++)
        key[i] = tolower(key[i]);
    return key;
}
/**************************************************************************************/
BotConfig::BotConfig(BotConfig *old)
{
  SET_SEGV_LOCATION();
  Flux::string conffile = binary_dir + "/bot.conf";
  try
  {
    this->Parser = new INIReader(conffile);
    this->Read();
  }
  catch(const ConfigException &e)
  {
    if (!old)
      throw; //Let CoreExcepion catch it since its OnStart
    else
      Log(LOG_TERMINAL) << "Config Exception: " << e.GetReason();
    return;
  }
}

BotConfig::~BotConfig() { if(Parser) delete Parser; }

void BotConfig::Read()
{
  SET_SEGV_LOCATION();
  this->LogFile 	= this->Parser->Get("Log","Log_File","navn.log");
  this->Realname 	= this->Parser->Get("Connect","Realname", Flux::string("ANT bot "+value_cast<Flux::string>(VERSION)+" (http://ANT.Flux-Net.net/)"));
  this->Ident 		= this->Parser->Get("Connect","Ident","ANT");
  this->Channel 	= this->Parser->Get("Connect","Channel","#Test");
  this->NicknamePrefix 	= this->Parser->Get("Bot", "Nickname Prefix", "ANT-");
  this->LogChannel 	= this->Parser->Get("Modules", "LogChannel","");
  this->PidFile 	= this->Parser->Get("Bot","PID File","navn.pid");
  this->ModuleDir 	= this->Parser->Get("Modules", "ModuleDir", "");
  this->xmlrpcbindip 	= this->Parser->Get("XML-RPC", "BindAddress", "0.0.0.0");
  this->jsonrpcbindip  = this->Parser->Get("JSON-RPC", "BindAddress", "0.0.0.0");
  this->LogTime 	= this->Parser->GetInteger("Log", "Log Time", 0);
  this->xmlrpcport 	= this->Parser->GetInteger("XML-RPC", "Port", 3910);
  this->xmlrpcipv6 	= this->Parser->GetBoolean("XML-RPC", "IPv6", false);
  this->xmlrpctimeout 	= this->Parser->GetInteger("XML-RPC", "Timeout", 30);
  this->jsonrpctimeout = this->Parser->GetInteger("JSON-RPC", "Timeout", 30);
  this->jsonrpcport    = this->Parser->GetInteger("JSON-RPC", "Port", 3920);
  this->jsonrpcipv6    = this->Parser->GetBoolean("JSON-RPC", "IPv6", false);
  this->UseIPv6 	= this->Parser->GetBoolean("Connect", "UseIPv6", false);
  this->SockWait 	= this->Parser->GetInteger("Socket","Socket Timeout",5);
  this->RetryWait 	= this->Parser->GetInteger("Socket", "Retry Wait", 30);
  this->dbforce 	= this->Parser->GetBoolean("Bot", "Force Database Read", false);
  this->LogColor	= this->Parser->Get("Log", "Color", "\033[0m").replace_all_cs("\\033", "\033");
  this->LogAge 		= this->Parser->GetInteger("Log", "Log Age", 2);
  this->LogChan		= this->Parser->Get("Log", "LogChan", "");
  this->LogChanNet	= this->Parser->Get("Log", "LogChanNet", "");
  this->DNSTimeout	= this->Parser->GetInteger("DNS", "Timeout", 30);
  this->NameServer	= this->Parser->Get("DNS", "NameServer", "127.0.0.1");
  this->sqlpass 	= this->Parser->Get("MySQL", "password", "");
  this->sqldb 		= this->Parser->Get("MySQL", "database", "");
  this->sqluser 	= this->Parser->Get("MySQL", "user", "");
  this->sqlhost		= this->Parser->Get("MySQL", "hostname", "127.0.0.1");
  this->sqlport		= this->Parser->GetInteger("MySQL", "hostname", 0);
  this->BurstRate	= this->Parser->GetInteger("SendQ", "Burst Rate", 7);
  this->SendQLines	= this->Parser->GetInteger("SendQ", "Lines", 5);
  this->SendQRate	= this->Parser->GetInteger("SendQ", "Rate", 5);
  this->SendQEnabled	= this->Parser->GetBoolean("SendQ", "Enabled", true);
  this->Modules 	= this->Parser->Modules;
}

