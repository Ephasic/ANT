#ifndef XMLFILE_H
#define XMLFILE_H
#include "module.h"

class CoreExport XMLFile : public TextFile {
private:
  class Tag
  {
  public:
    class Attribute
    {
    private:
    public:
      Attribute();
      Attribute(Flux::string);
      Flux::string Name;
      Flux::string Value;
    };
    Tag();
    Tag(Flux::string,Flux::string);
    Flux::string Name;
    Flux::string Content;
    Flux::map<Attribute> Attributes;
    Flux::map<Tag> Tags;
  };
public:
  XMLFile(Flux::string);
  /* The int here is just used to say that it is to use a different constructor */
  XMLFile(const Flux::string&, int);
  void Parse();
  Flux::map<Tag> Tags;
};

#endif