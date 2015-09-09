#ifndef FILE_DESCRIPTOR_HH
#define FILE_DESCRIPTOR_HH

#include <string>

#include "io_device.hh"

/* Unix file descriptors (sockets, files, etc.) */
class FileDescriptor : public IODevice
{
private:
  int fd_ = -1;
  bool eof_ = false;

protected:
  /* io device state */
  bool get_eof( void ) const noexcept override { return eof_; }
  void set_eof( void ) noexcept override { eof_ = true; }
  void reset_eof( void ) noexcept override { eof_ = false; }

  void close( void ) noexcept;

public:
  /* construct from fd number */
  FileDescriptor( int fd ) noexcept;

  /* move */
  FileDescriptor( FileDescriptor && other ) noexcept;
  FileDescriptor & operator=( FileDescriptor && other ) noexcept;

  /* forbid copying */
  FileDescriptor( const FileDescriptor & other ) = delete;
  FileDescriptor & operator=( const FileDescriptor & other ) = delete;

  /* destructor */
  virtual ~FileDescriptor() noexcept { close(); };

  /* accessors */
  int fd_num( void ) const noexcept { return fd_; }

  /* implement (p)read + (p)write */
  size_t read( char * buf, size_t limit ) override;
  size_t write( const char * buf, size_t nbytes ) override;
  size_t pread( char * buf, size_t limit, off_t offset ) override;
  size_t pwrite( const char * buf, size_t nbytes, off_t offset ) override;
};

#endif /* FILE_DESCRIPTOR_HH */

