#ifndef FILE_DESCRIPTOR_HH
#define FILE_DESCRIPTOR_HH

#include <string>

template <typename IOType> class BufferedIO;

/* Unix file descriptors (sockets, files, etc.) */
class FileDescriptor
{
public:
  template <typename IOType> friend class BufferedIO;
  using iterator_type = std::string::const_iterator;

protected:
  int fd_;
  bool eof_ = false;
  unsigned int read_count_ = 0, write_count_ = 0;

#ifdef __APPLE__
  /* default stack size on OSX is only 512KB, so need to lower */
  const static size_t MAX_READ_SIZE = 1024 * 256;
#else
  const static size_t MAX_READ_SIZE = 1024 * 1024;
#endif

  void close( void ) noexcept;

  void register_read( void ) noexcept { read_count_++; }
  void register_write( void ) noexcept { write_count_++; }
  void set_eof( void ) noexcept { eof_ = true; }
  void reset_eof( void ) noexcept { eof_ = false; }

  /* base read and write methods */
  virtual std::string rread( size_t limit = MAX_READ_SIZE );
  virtual ssize_t wwrite( const char * buffer, size_t count );

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
  const int & fd_num( void ) const noexcept { return fd_; }
  const bool & eof( void ) const noexcept { return eof_; }
  unsigned int read_count( void ) const noexcept { return read_count_; }
  unsigned int write_count( void ) const noexcept { return write_count_; }

  /* read method */
  std::string read( size_t limit = MAX_READ_SIZE, bool read_all = false );

  /* write methods */
  iterator_type write( const std::string & buffer, bool write_all = true );
  ssize_t write( const char * buffer, size_t count, bool write_all = true );
};

#endif /* FILE_DESCRIPTOR_HH */
