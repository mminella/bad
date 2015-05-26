#ifndef FILE_HH
#define FILE_HH

#include "file_descriptor.hh"

/* Unix file descriptors (sockets, files, etc.) */
class File : public FileDescriptor
{
public:
  /* construct from fd number */
  File( const int fd );

  /* construct by opening file at path given */
  File( const char * path, int flags );
  File( const char * path, int flags, mode_t mode );

  /* move constructor */
  File( File && other );

  /* destructor */
  virtual ~File();

  /* forbid copying FileDescriptor objects or assigning them */
  File( const File & other ) = delete;
  const File & operator=( const File & other ) = delete;
};

#endif /* FILE_HH */
