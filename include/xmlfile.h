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
    Flux::insensitive_map<Attribute> Attributes;
    Flux::insensitive_map<Tag> Tags;
  };
public:
  XMLFile(Flux::string);
  /* The int here is just used to say that it is to use a different constructor */
  XMLFile(const Flux::string&, int);
  void Parse();
  Flux::insensitive_map<Tag> Tags;
};

#endif