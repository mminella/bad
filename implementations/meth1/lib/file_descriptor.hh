#ifndef FILE_DESCRIPTOR_HH
#define FILE_DESCRIPTOR_HH

#include <string>

#include "iodevice.hh"

/* Unix file descriptors (sockets, files, etc.) */
class FileDescriptor : public IODevice
{
public:
  /* construct from fd number */
  FileDescriptor( const int fd );

  /* move constructor */
  FileDescriptor( FileDescriptor && other );

  /* destructor */
  virtual ~FileDescriptor();

  /* accessors */
  const int & fd_num( void ) const { return fd_r_; }

  /* forbid copying FileDescriptor objects or assigning them */
  FileDescriptor( const FileDescriptor & other ) = delete;
  const FileDescriptor & operator=( const FileDescriptor & other ) = delete;
};

#endif /* FILE_DESCRIPTOR_HH */
