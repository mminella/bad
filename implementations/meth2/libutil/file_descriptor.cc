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
size_t FileDescriptor::read( char * buf, size_t limit )
{
  if ( limit == 0 ) {
    throw runtime_error( "asked to read 0" );
  }

  size_t n = SystemCall( "read", ::read( fd_num(), buf, limit ) );
  if ( n == 0 ) {
    set_eof();
  }
  register_read();

  return n;
}

/* overriden base write method */
size_t FileDescriptor::write( const char * buf, size_t nbytes )
{
  if ( nbytes == 0 ) {
    throw runtime_error( "nothing to write" );
  } else if ( buf == nullptr ) {
    throw runtime_error( "null buffer for write" );
  }

  size_t n = SystemCall( "write", ::write( fd_num(), buf, nbytes ) );
  if ( n == 0 ) {
    throw runtime_error( "write returned 0" );
  }
  register_write();

  return n;
}

/* overriden base read method */
size_t FileDescriptor::pread( char * buf, size_t limit, off_t offset )
{
  if ( limit == 0 ) {
    throw runtime_error( "asked to read 0" );
  }

  size_t n = SystemCall( "pread", ::pread( fd_num(), buf, limit, offset ) );
  if ( n == 0 ) {
    set_eof();
  }
  register_read();

  return n;
}

/* overriden base write method */
size_t FileDescriptor::pwrite( const char * buf, size_t nbytes, off_t offset )
{
  if ( nbytes == 0 ) {
    throw runtime_error( "nothing to write" );
  } else if ( buf == nullptr ) {
    throw runtime_error( "null buffer for write" );
  }

  size_t n = SystemCall( "pwrite", ::pwrite( fd_num(), buf, nbytes, offset ) );
  if ( n == 0 ) {
    throw runtime_error( "write returned 0" );
  }
  register_write();

  return n;
}

