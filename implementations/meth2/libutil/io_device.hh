#ifndef IO_DEVICE_HH
#define IO_DEVICE_HH

#include <string>

/* Abstraction for IO capable device */
class IODevice
{
public:
  static constexpr size_t MAX_READ = 128 * 1024;

private:
  size_t read_count_ = 0;
  size_t write_count_ = 0;
  
protected:
  /* io device state */
  virtual bool get_eof( void ) const noexcept = 0;
  virtual void set_eof( void ) noexcept = 0;
  virtual void reset_eof( void ) noexcept = 0;

  /* device stats */
  void register_read( void ) noexcept { read_count_++; }
  void register_write( void ) noexcept { write_count_++; }

public:
  /* destructor */
  virtual ~IODevice() {};

  /* accessors */
  bool eof( void ) const noexcept { return get_eof(); }
  size_t read_count( void ) const noexcept { return read_count_; }
  size_t write_count( void ) const noexcept { return write_count_; }

  /* read methods */
  virtual size_t read( char * buf, size_t limit ) = 0;
  std::string read( size_t limit = MAX_READ );
  size_t read_all( char * buf, size_t nbytes );
  std::string read_all( size_t nbytes );

  virtual size_t pread( char * buf, size_t limit, off_t offset ) = 0;
  std::string pread( size_t limit, off_t offset );
  size_t pread_all( char * buf, size_t nbytes, off_t offset );
  std::string pread_all( size_t nbytes, off_t offset );

  /* write methods */
  virtual size_t write( const char * buf, size_t nbytes ) = 0;
  std::string::const_iterator write( const std::string & buf );
  size_t write_all( const char * buf, size_t nbytes );
  std::string::const_iterator write_all( const std::string & buf );

  virtual size_t pwrite( const char * buf, size_t nbytes, off_t offset ) = 0;
  std::string::const_iterator pwrite( const std::string & buf, off_t offset );
};

#endif /* IO_DEVICE_HH */
