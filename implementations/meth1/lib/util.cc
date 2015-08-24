#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>

#include <algorithm>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <string>
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

/* Convert a string to a bool */
bool to_bool( std::string str )
{
  std::transform( str.begin(), str.end(), str.begin(), ::tolower );
  std::istringstream is( str );
  bool b;
  is >> std::boolalpha >> b;
  return b;
}

/* Collapse a vector of strings to a single spaced string */
const string join( const vector<string> & command )
{
  return accumulate(
    command.begin() + 1, command.end(), command.front(),
    []( const string & a, const string & b ) { return a + " " + b; } );
}

/* Physical memory present on the machine */
uint64_t memory_exists( void )
{
  uint64_t pages = sysconf( _SC_PHYS_PAGES );
  uint64_t pg_sz = sysconf( _SC_PAGE_SIZE );
  return pages * pg_sz;
}

/* Physical memory present and free on the machine */
uint64_t memory_free( void )
{
  uint64_t pages = sysconf( _SC_AVPHYS_PAGES );
  uint64_t pg_sz = sysconf( _SC_PAGE_SIZE );
  return pages * pg_sz;
}
