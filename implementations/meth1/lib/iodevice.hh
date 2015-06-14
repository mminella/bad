#ifndef IODEVICE_HH
#define IODEVICE_HH

#include <string>

/* Device capable of IO (sockets, files, etc.) both for read and write. We
 * represent as two underlying file descriptors, a read and write one.
 */
class IODevice
{
public:
  using iterator_type = std::string::const_iterator;

protected:
  int fd_r_;
  int fd_w_;
  bool eof_;

  unsigned int read_count_, write_count_;

#ifdef __APPLE__
  /* default stack size on OSX is only 512KB, so need to lower */
  const static size_t BUFFER_SIZE = 1024 * 256;
#else
  const static size_t BUFFER_SIZE = 1024 * 1024;
#endif

  void register_read( void ) noexcept { read_count_++; }
  void register_write( void ) noexcept { write_count_++; }
  void set_eof( void ) noexcept { eof_ = true; }
  void reset_eof( void ) noexcept { eof_ = false; }

  /* base read and write methods */
  virtual std::string rread( size_t limit = BUFFER_SIZE );
  virtual ssize_t wwrite( const char * buffer, size_t count );

public:
  /* Construct an IO Device */
  IODevice( int fd_r, int fd_w ) noexcept;

  /* Forbid copy and move - subclasses should implement */
  IODevice( const IODevice & other ) = delete;
  IODevice( IODevice && other ) = delete;
  IODevice & operator=( const IODevice & other ) = delete;
  IODevice & operator=( IODevice && other ) = delete;

  /* Destruct an IO Device. Does nothing, but subclasses are expected to close
   * open file descriptors appropriately. */
  virtual ~IODevice() noexcept {};

  /* accessors */
  const int & fd_r_num( void ) const noexcept { return fd_r_; }
  const int & fd_w_num( void ) const noexcept { return fd_w_; }
  const bool & eof( void ) const noexcept { return eof_; }
  unsigned int read_count( void ) const noexcept { return read_count_; }
  unsigned int write_count( void ) const noexcept { return write_count_; }

  /* read method */
  std::string read( size_t limit = BUFFER_SIZE, bool read_all = false );

  /* write methods */
  ssize_t write( const char * buffer, size_t count, bool write_all = true );
  iterator_type write( const std::string & buffer, bool write_all = true );
  iterator_type write( const iterator_type & begin,
                       const iterator_type & end );
};

#endif /* IODEVICE_HH */
