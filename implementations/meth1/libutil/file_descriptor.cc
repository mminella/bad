#include <unistd.h>

#include <algorithm>
#include <string>

#include "file_descriptor.hh"
#include "exception.hh"

using namespace std;

/* construct from fd number */
FileDescriptor::FileDescriptor( int fd ) noexcept : fd_{fd} {}

/* move constructor */
FileDescriptor::FileDescriptor( FileDescriptor && other ) noexcept
{
  *this = move( other );
}

/* move assignment */
FileDescriptor & FileDescriptor::operator=( FileDescriptor && other ) noexcept
{
  if ( this != &other ) {
    IODevice::operator=( move( other ) );
    close();
    swap( fd_, other.fd_ );
  }
  return *this;
}

/* close the file descriptor */
void FileDescriptor::close( void ) noexcept
{
  if ( fd_ < 0 ) { /* has already been moved away */
    return;
  }

  try {
    SystemCall( "close", ::close( fd_ ) );
    fd_ = -1;
  } catch ( const exception & e ) { /* don't throw from destructor */
    print_exception( e );
  }
}

/* overriden base read method */
size_t FileDescriptor::rread( char * buf, size_t limit )
{
  return SystemCall( "read", ::read( fd_num(), buf, limit ) );
}

/* overriden base write method */
size_t FileDescriptor::wwrite( const char * buf, size_t nbytes )
{
  return SystemCall( "write", ::write( fd_num(), buf, nbytes ) );
}
