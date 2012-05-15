/* Arbitrary Navn Tool -- Internal Module Handler
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

/**
 * \fn bool ModuleHandler::Attach(Implementation i, module *mod)
 * \brief Module hook for the FOREACH_MOD macro
 * \param Implementation The Implementation of the call list you want your module to have
 * \param Module the module the Implementation is on
 */
bool ModuleHandler::Attach(Implementation i, module *mod)
{
  if(std::find(EventHandlers[i].begin(), EventHandlers[i].end(), mod) != EventHandlers[i].end())
    return false;

  EventHandlers[i].push_back(mod);
  return true;
}

/// \overload bool ModuleHandler::Attach(Implementation *i, module *mod, size_t sz)
void ModuleHandler::Attach(Implementation *i, module *mod, size_t sz)
{
  for(size_t n = 0; n < sz; ++n)
    Attach(i[n], mod);
}

Flux::string DecodeModErr(ModErr err)
{
  switch(err)
  {
    case MOD_ERR_OK:
      return "No error (MOD_ERR_OK)";
    case MOD_ERR_MEMORY:
      return "Out of memory (MOD_ERR_MEMORY)";
    case MOD_ERR_PARAMS:
      return "Insufficient parameters (MOD_ERR_PARAMS)";
    case MOD_ERR_EXISTS:
      return "Module Exists (MOD_ERR_EXISTS)";
    case MOD_ERR_NOEXIST:
      return "Module does not exist (MOD_ERR_NOEXIST)";
    case MOD_ERR_NOLOAD:
      return "Module cannot be loaded (MOD_ERR_NOLOAD)";
    case MOD_ERR_UNKNOWN:
      return "Unknown error (MOD_ERR_UNKNOWN)";
    case MOD_ERR_FILE_IO:
      return "File I/O Error (MOD_ERR_FILE_IO)";
    case MOD_ERR_EXCEPTION:
      return "Module Exception caught (MOD_ERR_EXCEPTION)";
    default:
      return "Unknown error code";
  }
}

/*  This code was found online at http://www.linuxjournal.com/article/3687#comment-26593 */
template<class TYPE> TYPE class_cast(void *symbol)
{
  union
  {
    void *symbol;
    TYPE function;
  } cast;
  cast.symbol = symbol;
  return cast.function;
}

/**
 * \fn bool ModuleHandler::Detach(Implementation i, module *mod)
 * \brief Unhook for the module hook ModuleHandler::Attach()
 * \param Implementation The Implementation of the call list you want your module to detach
 * \param Module the module the Implementation is on
 */
bool ModuleHandler::Detach(Implementation i, module *mod)
{
  auto x = std::find(EventHandlers[i].begin(), EventHandlers[i].end(), mod);
  if(x == EventHandlers[i].end())
    return false;
  EventHandlers[i].erase(x);
  return true;
}

void ModuleHandler::DetachAll(module *m)
{
  for(size_t n = I_BEGIN+1; n != I_END; ++n)
    Detach(static_cast<Implementation>(n), m);
}

ModErr ModuleHandler::LoadModule(const Flux::string &modname)
{
  SET_SEGV_LOCATION();

  if(modname.empty())
    return MOD_ERR_PARAMS;

  if(FindModule(modname))
    return MOD_ERR_EXISTS;

  Log() << "\033[0m[\033[1;32m*\033[0m] Loading module:\t\033[1;32m" << modname << Config->LogColor;

  Flux::string mdir = Config->Binary_Dir + "/runtime/"+ (modname.search(".so")?modname+".XXXXXX":modname+".so.XXXXXX");
  Flux::string input = Flux::string(Config->Binary_Dir + "/" + (Config->ModuleDir.empty()?modname:Config->ModuleDir+"/"+modname) + ".so").replace_all_cs("//","/");

  TextFile mod(input);
  Flux::string output = TextFile::TempFile(mdir);
  Log(LOG_RAWIO) << "Runtime module location: " << output;

  mod.Copy(output);
  if(mod.GetLastError() != FILE_IO_OK)
  {
    Log(LOG_RAWIO) << "Runtime Copy Error: " << mod.DecodeLastError();
    return MOD_ERR_FILE_IO;
  }

  dlerror();

  // FIXME: Somehow the binary_dir variable is lost when this executes >:|
  void *handle = dlopen(output.c_str(), RTLD_LAZY);
  const char *err = dlerror();

  if(!handle && err && *err)
  {
    Log() << '[' << modname << "] " << err;
    return MOD_ERR_NOLOAD;
  }

  dlerror();

  module *(*f)(const Flux::string&) = class_cast<module *(*)(const Flux::string&)>(dlsym(handle, "ModInit"));
  err = dlerror();

  if(!f && err && *err)
  {
    Log() << "No module init function, moving on.";
    dlclose(handle);
    return MOD_ERR_NOLOAD;
  }

  if(!f)
    throw CoreException("Can't find module constructor, yet no moderr?");

  module *m;
  try
  {
    m = f(modname);
  }
  catch (const ModuleException &e)
  {
    Log() << "Error while loading " << modname << ": " << e.GetReason();
    return MOD_ERR_EXCEPTION;
  }

  m->filepath = output;
  m->filename = (modname.search(".so")?modname:modname+".so");
  m->handle = reinterpret_cast<void*>(handle); //we'll convert to auto later, for now reinterpret_cast.

  m->OnLoad();

  FOREACH_MOD(I_OnModuleLoad, OnModuleLoad(m));

  return MOD_ERR_OK;
}

bool ModuleHandler::DeleteModule(module *m)
{
  SET_SEGV_LOCATION();
  if (!m || !m->handle)
    return false;

  auto *handle = m->handle;
  Flux::string filepath = m->filepath;

  dlerror();

  void (*df)(module*) = class_cast<void (*)(module*)>(dlsym(m->handle, "ModunInit"));
  const char *err = dlerror();

  SET_SEGV_LOCATION();
  if (!df && err && *err)
  {
    Log(LOG_DEBUG) << "No destroy function found for " << m->name << ", chancing delete...";
    delete m; /* we just have to chance they haven't overwrote the delete operator then... */
  }

  if(!df)
  {
    Log() << "[" << m->name << ".so] Module has no destroy function? (wtf?)";
    return false;
  }
  else
    df(m);

  if(handle)
    if(dlclose(handle))
      Log() << "[" << m->name << ".so] " << dlerror();

    return true;
}

bool ModuleHandler::Unload(module *m)
{
  if(!m || m->GetPermanent())
    return false;

  FOREACH_MOD(I_OnModuleUnload, OnModuleUnload(m));
  return DeleteModule(m);
}

void ModuleHandler::UnloadAll()
{
  for(std::list<module*>::iterator it = Modules.begin(), it_end = Modules.end(); it != it_end;)
  {
    module *m = *it;
    ++it;
    // ignore Unload function because we're forcing a unload regardless of whether it's permanent or not.
    FOREACH_MOD(I_OnModuleUnload, OnModuleUnload(m));
    DeleteModule(m);
  }
  Modules.clear();
}

Flux::string ModuleHandler::DecodePriority(ModulePriority p)
{
  switch(p)
  {
    case PRIORITY_FIRST:
      return "FIRST";
    case PRIORITY_DONTCARE:
      return "DON'T CARE";
    case PRIORITY_LAST:
      return "LAST";
    default:
      return "";
  }
  return "";
}

void ModuleHandler::SanitizeRuntime()
{
  Log(LOG_DEBUG) << "Cleaning up runtime directory.";
  Flux::string dirbuf = Config->Binary_Dir+"/runtime/";

  if(!TextFile::IsDirectory(dirbuf))
  {
    if(mkdir(dirbuf.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
      throw CoreException(printfify("Error making new runtime directory: %s", strerror(errno)));
  }
  else
  {
    Flux::vector files = TextFile::DirectoryListing(dirbuf);
    for(auto it : files)
      Delete(Flux::string(dirbuf+it).c_str());
  }
}

/**
 * \fn module *FindModule(const Flux::string &name)
 * \brief Find a module in the module list
 * \param name A string containing the module name you're looking for
 */
module *FindModule(const Flux::string &name)
{
  for(auto it : Modules)
  {
    module *m = it;
    if(m->name.equals_ci(name))
      return m;
  }

  return nullptr;
}

/******************Configuration variables***********************/
/**Rehash void
 * \fn void ReadConfig()
 * This will re-read the config file values when told to do so
 */
void LoadModules()
{
  sepstream sep(Config->Modules, ',');
  Flux::string tok;
  while(sep.GetToken(tok))
  {
    tok.trim();
    ModErr e = ModuleHandler::LoadModule(tok);
    if(e != MOD_ERR_OK)
    {
      Log() << "\n\033[0m[\033[1;31m*\033[0m] " << tok << ": " << DecodeModErr(e) << Config->LogColor << "\n";
      throw CoreException("Module Load Error");
    }
  }
}
/******************End Configuration variables********************/
