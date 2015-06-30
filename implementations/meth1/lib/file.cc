#include <fcntl.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <tuple>

#include "file.hh"
#include "exception.hh"

using namespace std;

/* construct from fd number */
File::File( int fd )
  : FileDescriptor( fd )
{
}

/* construct by opening file at path given */
File::File( const char * path, int flags )
  : File( SystemCall( "open", open( path, flags ) ) )
{
}

/* construct by opening file at path given */
File::File( const char * path, int flags, mode_t mode )
  : File( SystemCall( "open", open( path, flags, mode ) ) )
{
}

/* rewind to begging of file */
void File::rewind( void )
{
  SystemCall( "lseek", lseek( fd_num(), 0, SEEK_SET ) );
  reset_eof();
}

tuple<const char *, size_t>
BufferedFile::internal_read( size_t limit, bool copy )
{
  if ( limit == 0 ) {
    throw runtime_error( "asked to read 0" );
  }

  /* can't return large segments than our buffer */
  limit = min( limit, sizeof( buf_ ) );

  /* can fill (at least partially) from cache */
  if ( start_ < end_ ) {
    if ( !copy ) { limit = min( limit, end_ - start_ ); }

    if ( limit <= end_ - start_ ) {
      /* can completely fill from cache */
      const char * str = buf_ + start_;
      start_ += limit;
      return make_tuple( str, limit );
    } else {
      /* move cached to start of buffer */
      memmove( buf_, buf_ + start_, end_ - start_ );
      end_ = end_ - start_;
      start_ = 0;
    }
  } else {
    start_ = end_ = 0;
  }

  /* cache empty, refill */
  ssize_t n = SystemCall( "read",
    ::read( fd_r_, buf_ + end_, sizeof( buf_ ) - end_ ) );
  if ( n == 0 ) {
    set_eof();
  }
  register_read();
  end_ += n;

  /* return from cache */
  limit = min( static_cast<size_t>( n ), limit );
  start_ += limit;
  return make_tuple( buf_, limit );
}

/* read method (internal) */
string BufferedFile::rread( size_t limit )
{
  size_t n;
  const char * str;

  tie( str, n ) = internal_read( limit );
  
  return string( str, n );
}
