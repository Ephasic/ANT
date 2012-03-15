/* Arbitrary Navn Tool -- Configuration Parser Prototype
 * 
 * (C) 2011-2012 Azuru
 * Contact us at Dev@Flux-Net.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

// Read an INI file into easy-to-access name/value pairs.

// inih and INIReader are released under the New BSD license (see LICENSE.txt).
// Go to the project home page for more info:
//
// http://code.google.com/p/inih/
// This INIReader has been modified by Justasic, it is NOT the one on google code!

#ifndef __INIREADER_H__
#define __INIREADER_H__
#include "flux.h"
#include "extern.h"
#include "textfile.h"

// Read an INI file into easy-to-access name/value pairs. (Note that I've gone
// for simplicity here rather than speed, but it should be pretty decent.)
class CoreExport INIReader
{
public:
    // Construct INIReader and parse given filename. See ini.h for more info
    // about the parsing.
    INIReader(const Flux::string&);

    // Return the result of ini_parse(), i.e., 0 on success, line number of
    // first error on parse error, or -1 on file open error.
    int ParseError();

    // Get a string value from INI file, returning default_value if not found.
    Flux::string Get(const Flux::string&, const Flux::string&, const Flux::string&);

    // Get an integer (long) value from INI file, returning default_value if
    // not found.
    long GetInteger(const Flux::string&, const Flux::string&, long);

    bool GetBoolean(const Flux::string&, const Flux::string&, bool);
    ~INIReader();

private:
    int _error;
    Flux::map<Flux::string> _values;
    static Flux::string MakeKey(const Flux::string&, const Flux::string&);

    // Parse the INI file
    int Parse(const Flux::string &filename);
};

class CoreExport BotConfig
{
public:
  BotConfig(const Flux::string &dir = binary_dir);
  virtual ~BotConfig();
  INIReader *Parser;
  Flux::string LogFile;
  Flux::string Binary_Dir;
  Flux::string NicknamePrefix;
  Flux::string Realname;
  Flux::string Ident;
  Flux::string Channel;
  Flux::string LogChannel;
  Flux::string PidFile;
  Flux::string UserPass;
  Flux::string ModuleDir;
  Flux::string Modules;
  Flux::string xmlrpcbindip;
  std::string dbpass;
  std::string dbname;
  std::string dbuser;
  int xmlrpcport;
  time_t xmlrpctimeout;
  bool xmlrpcipv6;
  bool UseIPv6;
  bool dbforce;
  size_t SockWait;
  int RetryWait;
private:
  void Read();
};

#endif  // __INIREADER_H__
