/* Arbitrary Navn Tool -- Configuration Parser Prototype
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

// inih and INIReader are released under the New BSD license (see LICENSE.txt).
// Go to the project home page for more info:
//
// http://code.google.com/p/inih/
// This INIReader has been modified by Justasic, it is NOT the one on google code!
#pragma once
#ifndef INIREADER_H
#define INIREADER_H
#include "extern.h"
#include "textfile.h"
#include "PosixPath.h"

// Read an INI file into easy-to-access name/value pairs. (Note that I've gone
// for simplicity here rather than speed, but it should be pretty decent.)
class INIReader
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

class BotConfig
{
public:
  BotConfig(BotConfig*);
  virtual ~BotConfig();
  INIReader *Parser;
  Flux::string LogFile;
  Flux::string LogChan;
  Flux::string LogChanNet;
  Flux::string LogColor;
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
  Flux::string NameServer;
  Flux::string sqlpass;
  Flux::string sqlhost;
  Flux::string sqldb;
  Flux::string sqluser;
  int BurstRate;
  int SendQLines;
  int SendQRate;
  int sqlport;
  int xmlrpcport;
  time_t xmlrpctimeout;
  time_t DNSTimeout;
  bool xmlrpcipv6;
  bool SendQEnabled;
  bool UseIPv6;
  bool dbforce;
  size_t SockWait;
  int RetryWait;
  int LogAge;
  size_t LogTime;
private:
  void Read();
};

#endif  // __INIREADER_H__
