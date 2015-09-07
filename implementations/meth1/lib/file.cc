#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <tuple>

#include "file.hh"
#include "exception.hh"

using namespace std;

/* construct by opening file at path given */
File::File( const string & path, int flags )
  : FileDescriptor( SystemCall( "open", ::open( path.c_str(), flags ) ) )
{
}

/* construct by opening file at path given */
File::File( const string & path, int flags, mode_t mode )
  : FileDescriptor( SystemCall( "open", ::open( path.c_str(), flags, mode ) ) )
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

/* file size */
uint64_t File::size( void ) const
{
  struct stat st;
  fstat( fd_num(), &st );
  off_t siz = st.st_size;
  if ( siz < 0 ) {
    throw new runtime_error( "file size is less than zero" );
  }
  return (uint64_t) siz;
}
