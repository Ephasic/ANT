/* Arbitrary Navn Tool -- Module Routines.
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

#include "module.h"
// This code sucks, you know it and I know it.
// Move on and call me an idiot later.
std::list<module*> Modules;
EventsVector ModuleHandler::EventHandlers[I_END];
/**
 * \fn module::module(Flux::string n)
 * \brief Module Constructor
 * \param name Name of the module
 * \param activated Wether the module is activated or not
 * \param priority The module priority
 */

module::module(const Flux::string &n): author(""), version(""), loadtime(time(NULL)), Priority(PRIORITY_DONTCARE), permanent(false), handle(nullptr), name(n), filename(""), filepath("")
{
  SET_SEGV_LOCATION();
  if(FindModule(this->name))
    throw ModuleException("Module already exists!");

  Modules.push_back(this);
  if(!nofork)
    Log() << "Loaded module " << n;
}

module::~module()
{
  SET_SEGV_LOCATION();
  Log(LOG_DEBUG) << "Unloading module " << this->name;
  ModuleHandler::DetachAll(this);

  auto it = std::find(Modules.begin(), Modules.end(), this);
  if(it != Modules.end())
    Modules.erase(it);
  else
    Log() << "Could not find " << this->name << " in module map!";
}

void module::SetAuthor(const Flux::string &person)
{
  this->author = person;
}

void module::SetVersion(const Flux::string &ver)
{
  this->version = ver;
}

void module::SetPriority(ModulePriority p)
{
  this->Priority = p;
}

void module::SetPermanent(bool state)
{
  this->permanent = state;
}

Flux::string module::GetVersion()
{
  return this->version;
}

time_t module::GetLoadTime()
{
  return this->loadtime;
}

Flux::string module::GetAuthor()
{
  return this->author;
}

ModulePriority module::GetPriority()
{
  return this->Priority;
}

bool module::GetPermanent()
{
  return this->permanent;
}

/******************Configuration variables***********************/
/**Rehash void
 * \fn void ReadConfig()
 * This will re-read the config file values when told to do so
 */
void ReadConfig()
{
  sepstream sep(Config->Modules, ',');
  Flux::string tok;
  while(sep.GetToken(tok))
  {
    tok.trim();
    ModErr e = ModuleHandler::LoadModule(tok);
    if(e != MOD_ERR_OK)
      Log() << "ERROR loading module " << tok << ": " << DecodeModErr(e);
  }
}
/******************End Configuration variables********************/