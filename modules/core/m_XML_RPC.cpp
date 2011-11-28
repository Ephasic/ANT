/*
 * This is the module file for the XML_RPC commits, this will handle 98% of the commits the bot will process!
 */
class xmlrpcmod : public module
{
public:
  xmlrpcmod(const Flux::string &name):module(Name)
  {
    this->SetAuthor("Justasic");
    this->SetVersion(VERSION_SIMPLE);
    Implementation i[] = { I_OnCommit };
    ModuleHandler::Attach(i, this, sizeof(i)/sizeof(Implementation));
  }
  void OnCommit(CommitMessage &msg)
  {
    Log(LOG_TERMINAL) << "Fun stuff!";
  }
};