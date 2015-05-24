#include <unistd.h>

#include <cassert>

#include "iodevice.hh"
#include "exception.hh"

using namespace std;

/* default constructor */
IODevice::IODevice( const int fd_r, const int fd_w )
  : fd_r_( fd_r )
  , fd_w_( fd_w )
  , eof_( false )
  , read_count_( 0 )
  , write_count_( 0 )
{
}

/* attempt to write a portion of a string */
string::const_iterator IODevice::write( const string::const_iterator & begin,
                                        const string::const_iterator & end )
{
  if ( begin >= end ) {
    throw runtime_error( "nothing to write" );
  }

  ssize_t bytes_written =
    SystemCall( "write", ::write( fd_w_, &*begin, end - begin ) );
  if ( bytes_written == 0 ) {
    throw runtime_error( "write returned 0" );
  }

  register_write();

  return begin + bytes_written;
}

/* read method */
string IODevice::read( const size_t limit )
{
  char buffer[BUFFER_SIZE];

  assert( limit > 0 );

  ssize_t bytes_read = SystemCall(
    "read", ::read( fd_r_, buffer, min( sizeof( buffer ), limit ) ) );
  if ( bytes_read == 0 ) {
    set_eof();
  }

  register_read();

  return string( buffer, bytes_read );
}

/* write method */
string::const_iterator IODevice::write( const std::string & buffer,
                                        const bool write_all )
{
  auto it = buffer.begin();

  do {
    it = write( it, buffer.end() );
  } while ( write_all and ( it != buffer.end() ) );

  return it;
}
