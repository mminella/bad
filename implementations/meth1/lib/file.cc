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
  : File( SystemCall( "open", ::open( path, flags ) ) )
{
}

/* construct by opening file at path given */
File::File( const char * path, int flags, mode_t mode )
  : File( SystemCall( "open", ::open( path, flags, mode ) ) )
{
}

/* rewind to begging of file */
void File::rewind( void )
{
  SystemCall( "lseek", ::lseek( fd_num(), 0, SEEK_SET ) );
  reset_eof();
}

/* force file contents to disk */
void File::fsync( void )
{
  SystemCall( "fsync", ::fsync( fd_num() ) );
}
