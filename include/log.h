#ifndef LOG_H
#define LOG_H
#include "command.h"
#include "user.h"

class CoreExport Log
{
protected:
  std::fstream log;
public:
  LogType type;
  User *u;
  Command *c;
  std::stringstream buffer;
  static Flux::string TimeStamp();
  Log(LogType type = LOG_NORMAL);
  Log(LogType, User*);
  Log(User*);
  Log(User*, Command*);
  Log(LogType, User*, Command*);
  ~Log();
  
  template<typename T> Log &operator<<(T val)
  {
   this->buffer << val;
   return *this;
  }
};
#endif