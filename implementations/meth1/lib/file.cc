#include <fcntl.h>
#include <unistd.h>

#include <cassert>

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

/* move constructor */
File::File( File && other )
  : FileDescriptor( move( other ) )
{
}

/* move assignment */
File & File::operator=( File && other )
{
  if ( this != &other ) {
    *static_cast<FileDescriptor *>( this ) =
      move( static_cast<FileDescriptor &&>( other ) );
  }
  return *this;
}

/* destructor */
File::~File() {}

/* rewind to begging of file */
void File::rewind( void )
{
  SystemCall( "lseek", lseek( fd_num(), 0, SEEK_SET ) );
  reset_eof();
}
