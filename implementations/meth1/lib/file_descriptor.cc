#include <unistd.h>

#include <cassert>

#include "file_descriptor.hh"
#include "exception.hh"

using namespace std;

/* construct from fd number */
FileDescriptor::FileDescriptor( int fd ) noexcept
  : IODevice( fd, fd )
{
}

/* move constructor */
FileDescriptor::FileDescriptor( FileDescriptor && other ) noexcept
  : IODevice( other.fd_r_, other.fd_w_ )
{
  /* mark other file descriptor as inactive */
  other.fd_r_ = -1;
  other.fd_w_ = -1;
}

/* move assignment */
FileDescriptor &
FileDescriptor::operator=( FileDescriptor && other ) noexcept
{
  if ( this != &other ) {
    close();
    swap( fd_r_, other.fd_r_ );
    swap( fd_w_, other.fd_w_ );
  }
  return *this;
}

/* destructor */
FileDescriptor::~FileDescriptor() noexcept
{
  close();
}

/* close the file descriptor */
void FileDescriptor::close( void ) noexcept
{
  if ( fd_r_ < 0 ) { /* has already been moved away */
    return;
  }

  try {
    SystemCall( "close", ::close( fd_r_ ) );
    fd_r_ = -1;
    fd_w_ = -1;
  } catch ( const exception & e ) { /* don't throw from destructor */
    print_exception( e );
  }
}
