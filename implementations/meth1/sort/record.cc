#include <cstring>

#include "record.hh"

Record::Record( const char * s )
{
  std::memcpy( key_, s, KEY_LEN );
  std::memcpy( value_, s, VAL_LEN );
}

std::string Record::str( void )
{
  char buf[KEY_LEN + VAL_LEN];
  std::memcpy( buf, key_, KEY_LEN );
  std::memcpy( buf + KEY_LEN, value_, VAL_LEN );
  return std::string( buf, KEY_LEN + VAL_LEN );
}

