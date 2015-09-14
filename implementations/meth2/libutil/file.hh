#ifndef FILE_HH
#define FILE_HH

#include <fcntl.h>

#include "file_descriptor.hh"

/* Unix file descriptors (sockets, files, etc.) */
class File : public FileDescriptor
{
public:
  /* construct by opening file at path given */
  File( const std::string & path, int flags, odirect_t od = CACHED );
  File( const std::string & path, int flags, mode_t m, odirect_t od = CACHED );

  /* rewind to begging of file */
  void rewind( void );

  /* force file contents to disk */
  void fsync( void );

  /* file size */
  off_t size( void ) const;
};

#endif /* FILE_HH */
