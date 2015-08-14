#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>

#include <numeric>
#include <vector>

#include "exception.hh"
#include "file_descriptor.hh"
#include "privs.hh"
#include "util.hh"

using namespace std;

/* Convert a string to hexadecimal form */
string str_to_hex( const string & in )
{
  const uint8_t * const buf =
    reinterpret_cast<const uint8_t * const>( in.c_str() );
  return str_to_hex( buf, in.length() );
}

string str_to_hex( const char * const in, size_t size )
{
  return str_to_hex( (const uint8_t * const) in, size );
}

/* Convert a string to hexadecimal form */
string str_to_hex( const uint8_t * const in, size_t size )
{
  if ( size <= 0 ) {
    return "";
  }

  static const char * const lut = "0123456789abcdef";

  string str;
  str.reserve( size * 2 );

  for ( size_t i = 0; i < size; i++ ) {
    const uint8_t c = in[i];
    str.push_back( lut[c >> 4] );
    str.push_back( lut[c & 15] );
  }

  return str;
}

/* Collapse a vector of strings to a single spaced string */
const string join( const vector<string> & command )
{
  return accumulate(
    command.begin() + 1, command.end(), command.front(),
    []( const string & a, const string & b ) { return a + " " + b; } );
}
