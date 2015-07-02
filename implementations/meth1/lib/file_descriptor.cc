#include <unistd.h>

#include <cassert>
#include <iostream>

#include "file_descriptor.hh"
#include "exception.hh"

using namespace std;

/* construct from fd number */
FileDescriptor::FileDescriptor( int fd ) noexcept : fd_{fd} {}

/* move constructor */
FileDescriptor::FileDescriptor( FileDescriptor && other ) noexcept
  : fd_{-1}
{
  *this = move( other );
}

/* move assignment */
FileDescriptor & FileDescriptor::operator=( FileDescriptor && other ) noexcept
{
  if ( this != &other ) {
    close();
    swap( fd_, other.fd_ );
    eof_ = other.eof_;
    read_count_ = other.read_count_;
    write_count_ = other.write_count_;
  }
  return *this;
}

/* destructor */
FileDescriptor::~FileDescriptor() noexcept { close(); }

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

/* read method (internal) */
string FileDescriptor::rread( size_t limit )
{
  char buffer[MAX_READ_SIZE];

  if ( limit == 0 ) {
    throw runtime_error( "asked to read 0" );
  }

  ssize_t bytes_read = SystemCall(
    "read", ::read( fd_, buffer, min( sizeof( buffer ), limit ) ) );
  if ( bytes_read == 0 ) {
    set_eof();
  }

  register_read();

  return string( buffer, bytes_read );
}

/* read method (external) */
string FileDescriptor::read( size_t limit, bool read_all )
{
  string str;
  if ( read_all and limit > 0 ) {
    str.reserve( limit );
  }

  do {
    str += rread( limit - str.size() );
  } while ( str.size() < limit and read_all );

  return str;
}

/* write a cstring (internal) */
ssize_t FileDescriptor::wwrite( const char * buffer, size_t count )
{
  if ( count == 0 ) {
    throw runtime_error( "nothing to write" );
  }

  ssize_t n = SystemCall( "write", ::write( fd_, buffer, count ) );
  if ( n == 0 ) {
    throw runtime_error( "write returned 0" );
  }

  register_write();

  return n;
}

/* write a cstring (external) */
ssize_t FileDescriptor::write( const char * buffer, size_t count, bool write_all )
{
  size_t n{0};

  do {
    n += wwrite( buffer + n, count - n );
  } while ( write_all and n < count );

  return n;
}

/* write string */
FileDescriptor::iterator_type
FileDescriptor::write( const std::string & buffer, bool write_all )
{
  auto it = buffer.begin();

  do {
    it += wwrite( &*it, buffer.end() - it );
  } while ( write_all and ( it != buffer.end() ) );

  return it;
}
