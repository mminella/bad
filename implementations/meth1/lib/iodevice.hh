#ifndef IODEVICE_HH
#define IODEVICE_HH

#include <string>

/* Device capable of IO (sockets, files, etc.). We represent as a union of two
 * underlying file descriptors, a read and write. */
class IODevice
{
protected:
  int fd_r_;
  int fd_w_;
  bool eof_;

  unsigned int read_count_, write_count_;

  const static size_t BUFFER_SIZE = 1024 * 1024;

  void register_read( void ) { read_count_++; }
  void register_write( void ) { write_count_++; }
  void set_eof( void ) { eof_ = true; }

public:
  IODevice( const int fd_r, const int fd_w );
  virtual ~IODevice(){};

  /* accessors */
  const int & fd_r_num( void ) const { return fd_r_; }
  const int & fd_w_num( void ) const { return fd_w_; }
  const bool & eof( void ) const { return eof_; }
  unsigned int read_count( void ) const { return read_count_; }
  unsigned int write_count( void ) const { return write_count_; }

  /* read and write methods */
  virtual std::string read( const size_t limit = BUFFER_SIZE );
  std::string::const_iterator write( const std::string & buffer,
                                     const bool write_all = true );

  /* attempt to write a portion of a string */
  virtual std::string::const_iterator
  write( const std::string::const_iterator & begin,
         const std::string::const_iterator & end );
};

#endif /* IODEVICE_HH */
