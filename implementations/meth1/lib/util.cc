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

/* Convert a string to hexadecimal form */
string str_to_hex( const uint8_t * const in, size_t size )
{
  if ( size <= 0 ) {
    return "";
  }

  static const char * const lut = "0123456789ABCDEF";

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

/* Retrieve path to users default shell */
string shell_path( void )
{
  const char * sh = getenv( "SHELL" );
  if ( sh != nullptr ) {
    string shell_path{sh};
    if ( !shell_path.empty() ) {
      return shell_path;
    }
  }

  passwd * pw = getpwuid( getuid() );
  if ( pw == nullptr ) {
    throw unix_error( "getpwuid" );
  }

  string shell_path( pw->pw_shell );
  if ( shell_path.empty() ) { /* empty shell means Bourne shell */
    shell_path = "/bin/bash";
  }

  return shell_path;
}

/* prepend a string to the user's shell PS1 */
void prepend_shell_ps1( const char * str )
{
  const char * old_prefix = getenv( "LIBTUN_SHELL_PREFIX" );
  string prefix = old_prefix ? old_prefix : "";
  prefix.append( str );

  SystemCall( "setenv",
              setenv( "LIBTUN_SHELL_PREFIX", prefix.c_str(), true ) );
  SystemCall( "setenv",
              setenv( "PROMPT_COMMAND",
                      "PS1=\"$LIBTUN_SHELL_PREFIX$PS1\" PROMPT_COMMAND=",
                      true ) );
}

/* verify args and stderr are present */
void sanity_check_env( const int argc )
{
  if ( argc <= 0 ) {
    /* really crazy user */
    throw runtime_error( "missing argv[ 0 ]: argc <= 0" );
  }

  /* verify normal fds are present (stderr hasn't been closed) */
  FileDescriptor{
    SystemCall( "open /dev/null", open( "/dev/null", O_RDONLY ) )};
}

/* verify IP forwarding is enabled */
void check_ip_forwarding( const string & prog )
{
  FileDescriptor ipf{
    SystemCall( "open /proc/sys/net/ipv4/ip_forward",
                open( "/proc/sys/net/ipv4/ip_forward", O_RDONLY ) )};

  if ( ipf.read() != "1\n" ) {
    throw runtime_error(
      prog + ": Please run \"sudo sysctl -w net.ipv4.ip_forward=1\" " +
      "to enable IP forwarding" );
  }
}
