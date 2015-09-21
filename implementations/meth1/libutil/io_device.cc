#include <unistd.h>

#include <algorithm>
#include <string>

#include "io_device.hh"
#include "exception.hh"

using namespace std;

/* virtual to allow performance improvements if io device already has an
 * internal buffer. */
// XXX: Above is no longer true. Which is now wrong? Comment or implementation?
string IODevice::read( size_t limit )
{
  char buf[MAX_READ];
  size_t n = read( buf, min( limit, sizeof( buf ) ) );
  return {buf, n};
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

string IODevice::pread( size_t limit, off_t offset )
{
  char buf[MAX_READ];
  size_t n = pread( buf, min( limit, sizeof( buf ) ), offset );
  return {buf, n};
}

size_t IODevice::pread_all( char * buf, size_t nbytes, off_t offset )
{
  size_t n = 0;

  do {
    n += pread( buf + n, nbytes - n, offset + n );
  } while ( n < nbytes and !eof() );

  return n;
}

string IODevice::pread_all( size_t nbytes, off_t offset )
{
  string str;
  if ( nbytes > 0 ) {
    str.reserve( nbytes );
  }

  do {
    str += pread( nbytes - str.size(), offset + str.size() );
  } while ( str.size() < nbytes and !eof() );

  return str;
}
/* write methods */
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

string::const_iterator IODevice::pwrite( const string & buf, off_t offset )
{
  auto it = buf.begin();
  it += pwrite( &*it, buf.end() - it, offset );
  return it;
}

