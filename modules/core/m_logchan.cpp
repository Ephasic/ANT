/* Arbitrary Navn Tool -- Log to channel
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

Flux::string NoTermColor(const Flux::string &ret)
{
  Flux::string str;
  bool in_term_color = false;
  for(unsigned i=0; i < ret.length(); ++i)
  {
    char c = ret[i];
    if(in_term_color)
    {
      if(c == 'm')
	in_term_color = false;
      continue;
    }
    if(c == '\033')
    {
      in_term_color = true;
      continue;
    }
    if(!in_term_color)
      str += c;
  }
  return str;
}

class LogChan : public module
{
public:
  LogChan(const Flux::string &Name) : module(Name)
  {
    this->SetAuthor("Justasic");
    this->SetVersion(VERSION);
    ModuleHandler::Attach(I_OnLog, this);

    if(Config->LogChan.empty())
      throw ModuleException("Log channel is empty, cannot load m_logchan!");
  }

  EventResult OnLog(Log *l)
  {
    Network *n = FindNetwork(Config->LogChanNet);
    if(!n || !n->IsSynced())
      return EVENT_CONTINUE;
    Channel *c = FindChannel(n, Config->LogChan);
    if(!c || l->type == LOG_RAWIO)
      return EVENT_CONTINUE;
    
    Flux::string message = l->buffer.str();
    if(l->u && !l->c)
      message = l->u->nick + " " + message;
    if(l->u && l->c)
      message = l->u->nick + " on " + l->u->n->name + " used " + l->c->name + " " + message;
    
    std::stringstream logstream;

    switch(l->type)
    {
      case LOG_NORMAL:
	logstream << message;
	break;
      case LOG_THREAD:
	if(protocoldebug)
	  logstream << "[THREAD] " << message;
	break;
      case LOG_DEBUG:
	if(dev || protocoldebug)
	  logstream << message;
	break;
      case LOG_DNS:
	if(protocoldebug)
	  logstream << "[DNSEngine] " << message;
	break;
      case LOG_RAWIO:
	if(protocoldebug)
	  logstream << message;
	break;
      case LOG_CRITICAL:
	logstream << "\0034[CRITICAL] " << message;
	break;
      case LOG_WARN:
	logstream << "\0037[WARNING]\017 " << message;
	break;
      case LOG_SILENT:
      case LOG_MEMORY:
      case LOG_TERMINAL:
	break;
      default:
	break;
    }
      
    c->SendMessage(NoTermColor(logstream.str()));
    return EVENT_CONTINUE;
  }
};

MODULE_HOOK(LogChan)