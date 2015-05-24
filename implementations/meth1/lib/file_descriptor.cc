#include <unistd.h>

#include <cassert>

#include "file_descriptor.hh"
#include "exception.hh"

using namespace std;

/* construct from fd number */
FileDescriptor::FileDescriptor( const int fd )
  : IODevice( fd, fd )
{
}

/* move constructor */
FileDescriptor::FileDescriptor( FileDescriptor && other )
  : IODevice( other.fd_r_, other.fd_w_ )
{
  /* mark other file descriptor as inactive */
  other.fd_r_ = -1;
  other.fd_w_ = -1;
}

/* destructor */
FileDescriptor::~FileDescriptor()
{
  if ( fd_r_ < 0 ) { /* has already been moved away */
    return;
  }

  try {
    SystemCall( "close", close( fd_r_ ) );
  } catch ( const exception & e ) { /* don't throw from destructor */
    print_exception( e );
  }
}
