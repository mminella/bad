#include <unistd.h>

#include <algorithm>
#include <string>

#include "io_device.hh"
#include "exception.hh"

using namespace std;

/* virtual to allow performance improvements if io device already has an
 * internal buffer. */
string IODevice::rread( size_t limit )
{
  char buf[MAX_READ];
  size_t n = rread( buf, min( limit, sizeof( buf ) ) );
  return {buf, n};
}

/* read methods */
size_t IODevice::read( char * buf, size_t limit )
{
  if ( limit == 0 ) {
    throw runtime_error( "asked to read 0" );
  }

  size_t n = rread( buf, limit );
  if ( n == 0 ) {
    set_eof();
  }
  register_read();

  return n;
}

string IODevice::read( size_t limit )
{
  if ( limit == 0 ) {
    throw runtime_error( "asked to read 0" );
  }

  string str = rread( limit );
  if ( str.size() == 0 ) {
    set_eof();
  }
  register_read();

  return str;
}

size_t IODevice::read_all( char * buf, size_t nbytes )
{
  size_t n = 0;

  do {
    n += read( buf + n, nbytes - n );
  } while ( n < nbytes and !eof() );

  return n;
}

string IODevice::read_all( size_t nbytes )
{
  string str;
  if ( nbytes > 0 ) {
    str.reserve( nbytes );
  }

  do {
    str += read( nbytes - str.size() );
  } while ( str.size() < nbytes and !eof() );

  return str;
}

/* write methods */
size_t IODevice::write( const char * buf, size_t nbytes )
{
  if ( nbytes == 0 ) {
    throw runtime_error( "nothing to write" );
  }

  size_t n = wwrite( buf, nbytes );
  if ( n == 0 ) {
    throw runtime_error( "write returned 0" );
  }
  register_write();

  return n;
}

string::const_iterator IODevice::write( const string & buf )
{
  auto it = buf.begin();
  it += write( &*it, buf.end() - it );
  return it;
}

size_t IODevice::write_all( const char * buf, size_t nbytes )
{
  size_t n = 0;
  do {
    n += write( buf + n, nbytes - n  );
  } while ( n < nbytes );
  return n;
}

string::const_iterator IODevice::write_all( const string & buf )
{
  auto it = buf.begin();
  it += write_all( &*it, buf.end() - it );
  return it;
}

