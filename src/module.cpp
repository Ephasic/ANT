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
std::list<Module*> Modules;
EventsVector ModuleHandler::EventHandlers[I_END];
/**
 * \fn Module::Module(Flux::string n)
 * \brief Module Constructor
 * \param name Name of the Module
 * \param activated Wether the Module is activated or not
 * \param priority The Module priority
 */

Module::Module(const Flux::string &n, ModType m): author(""), version(""), loadtime(time(NULL)), Priority(PRIORITY_DONTCARE), permanent(false), handle(nullptr), name(n), filename(""), filepath("")
{
  SET_SEGV_LOCATION();
  if(FindModule(this->name))
    throw ModuleException("Module already exists!");

  Modules.push_back(this);
  if(!nofork)
    Log() << "Loaded Module " << n;
}

Module::~Module()
{
  SET_SEGV_LOCATION();
  Log(LOG_DEBUG) << "Unloading Module " << this->name;
  ModuleHandler::DetachAll(this);

  auto it = std::find(Modules.begin(), Modules.end(), this);
  if(it != Modules.end())
    Modules.erase(it);
  else
    Log() << "Could not find " << this->name << " in Module map!";
}

void Module::SetAuthor(const Flux::string &person)
{
  this->author = person;
}

void Module::SetVersion(const Flux::string &ver)
{
  this->version = ver;
}

void Module::SetPriority(ModulePriority p)
{
  this->Priority = p;
}

void Module::SetPermanent(bool state)
{
  this->permanent = state;
}