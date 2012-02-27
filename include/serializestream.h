/* Arbitrary Navn Tool -- Serialization Streams
 *
 * (C) 2011-2012 Flux-Net
 * Contact us at Dev@Flux-Net.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

#ifndef _SERIALIZESTREAM_H_
#define _SERIALIZESTREAM_H_

#include "include.h"
#include "flux.h"
#include <sstream>

class SerializeStream : std::stringstream
{
public:
  SerializeStream();
  SerializeStream(const SerializeStream&);

  template<typename T> std::istream &operator>>(T &val)
  {
    std::istringstream is(this->str());
    is >> val;
    return *this;
  }

  std::istream &operator>> (Flux::string&);
};

typedef std::map<Flux::string, SerializeStream> SerializeMap;

#endif // _SERIALIZESTREAM_H_