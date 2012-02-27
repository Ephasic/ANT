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

#include "serializestream.h"

SerializeStream::SerializeStream() : std::stringstream() {}
SerializeStream::SerializeStream(const SerializeStream &ss) : std::stringstream(ss.str()) {}

std::istream &SerializeStream::operator>>(Flux::string &val);
{
  val = this->str();
  return *this;
}

