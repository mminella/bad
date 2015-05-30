#ifndef FILE_DESCRIPTOR_HH
#define FILE_DESCRIPTOR_HH

#include <string>

#include "iodevice.hh"

/* Unix file descriptors (sockets, files, etc.) */
class FileDescriptor : public IODevice
{
protected:
  void close( void ) noexcept;

public:
  /* construct from fd number */
  FileDescriptor( int fd ) noexcept;

  /* move */
  FileDescriptor( FileDescriptor && other ) noexcept;
  FileDescriptor & operator=( FileDescriptor && other ) noexcept;

  /* forbid copying FileDescriptor objects */
  FileDescriptor( const FileDescriptor & other ) = delete;
  FileDescriptor & operator=( const FileDescriptor & other ) = delete;

  /* destructor */
  virtual ~FileDescriptor() noexcept;

  /* accessors */
  const int & fd_num( void ) const noexcept { return fd_r_; }
};

#endif /* FILE_DESCRIPTOR_HH */
