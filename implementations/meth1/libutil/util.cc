#include <fcntl.h>
#include <pwd.h>
#ifdef __APPLE__
#include <sys/sysctl.h>
#endif
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

/* Ensure size_t is large enough */

/* Physical memory present on the machine */
size_t memory_exists( void )
{
#ifdef __APPLE__
  static_assert( sizeof( size_t ) >= sizeof( int64_t ), "size_t >= int64_t" );

  int64_t mem;
  size_t len = sizeof(mem);
  sysctlbyname("hw.memsize", &mem, &len, NULL, 0);
  if ( mem < 0 ) {
    throw runtime_error( "memory available less than zero" );
  }
  return (size_t) mem;
#else
  static_assert( sizeof( size_t ) >= sizeof( long ), "size_t >= long" );

  size_t pages = (size_t) sysconf( _SC_PHYS_PAGES );
  size_t pg_sz = (size_t) sysconf( _SC_PAGE_SIZE );
  return pages * pg_sz;
#endif
}

/* Physical memory present and free on the machine */
size_t memory_free( void )
{
#ifdef __APPLE__
  return memory_exists();
#else
  static_assert( sizeof( size_t ) >= sizeof( long ), "size_t >= long" );

  size_t pages = (size_t) sysconf( _SC_AVPHYS_PAGES );
  size_t pg_sz = (size_t) sysconf( _SC_PAGE_SIZE );
  return pages * pg_sz;
#endif
}

/* Return your hostname */
std::string my_host_name( void )
{
  long host_max = sysconf( _SC_HOST_NAME_MAX );
  if ( host_max <= 0 ) {
#ifdef HOST_NAME_MAX
    host_max = HOST_NAME_MAX;
#else
    host_max = _POSIX_HOST_NAME_MAX;
#endif
  }
  std::vector<char> hostname( host_max + 1 );
  gethostname( hostname.data(), host_max + 1 );
  return {hostname.data()};
}
