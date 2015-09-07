#ifndef FILE_HH
#define FILE_HH

#include <fcntl.h>

#include "file_descriptor.hh"

/* Unix file descriptors (sockets, files, etc.) */
class File : public FileDescriptor
{
public:
  /* construct by opening file at path given */
  File( const std::string & path, int flags );
  File( const std::string & path, int flags, mode_t mode );

  /* rewind to begging of file */
  void rewind( void );

  /* Seek */
  void seek( uint64_t off );

  /* force file contents to disk */
  void fsync( void );

  /* file size */
  size_t size( void );
};

#endif /* FILE_HH */
