#ifndef IO_DEVICE_HH
#define IO_DEVICE_HH

#include <string>

/* Abstraction for IO capable device */
class IODevice
{
public:
  static const size_t MAX_READ = 128 * 1024;

private:
  unsigned int read_count_ = 0;
  unsigned int write_count_ = 0;
  
  /* base read & write methods */
  virtual size_t rread( char * buf, size_t limit ) = 0;
  virtual std::string rread( size_t limit = MAX_READ );
  virtual size_t wwrite( const char * buf, size_t nbytes ) = 0;

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
  unsigned int read_count( void ) const noexcept { return read_count_; }
  unsigned int write_count( void ) const noexcept { return write_count_; }

  /* read methods */
  size_t read( char * buf, size_t limit );
  std::string read( size_t limit = MAX_READ );
  size_t read_all( char * buf, size_t nbytes );
  std::string read_all( size_t nbytes );

  /* write methods */
  size_t write( const char * buf, size_t nbytes );
  std::string::const_iterator write( const std::string & buf );
  size_t write_all( const char * buf, size_t nbytes );
  std::string::const_iterator write_all( const std::string & buf );
};

#endif /* IO_DEVICE_HH */
