#ifndef BUFFERED_IO_HH
#define BUFFERED_IO_HH

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "io_device.hh"

/* Wrapper around an IODevice to buffer reads and/or writes. */
class BufferedIO : public IODevice
{
private:
  /* buffers */
  std::unique_ptr<char> rbuf_;
  std::unique_ptr<char> wbuf_;
  size_t rstart_ = 0, rend_ = 0;
  size_t wstart_ = 0, wend_ = 0;

  /* underlying file descriptor */
  IODevice & io_;

  /* base buffer read method */
  std::pair<const char *, size_t> rread_buf( size_t limit, bool read_all );

  /* implement read + write */
  size_t rread( char * buf, size_t limit ) override;
  std::string rread( size_t limit = MAX_READ ) override;
  size_t wwrite( const char * buf, size_t nbytes ) override;

protected:
  /* io device state */
  bool get_eof( void ) const noexcept override { return io_.eof(); }
  void set_eof( void ) noexcept override {}
  void reset_eof( void ) noexcept override {}

public:
  static const size_t BUFFER_SIZE = 4096;

  /* constructor */
  BufferedIO( IODevice & io );

  /* buffer read method */
  std::pair<const char *, size_t> read_buf( size_t limit = BUFFER_SIZE );
  std::pair<const char *, size_t> read_buf_all( size_t nbytes );

  /* flush */
  size_t flush( bool flush_all = true );

  /* destructor */
  ~BufferedIO() { flush( true ); }
};

/* A version of BufferedIO that takes ownership of the IODevice */
template <typename IOType>
class BufferedIO_O : public BufferedIO
{
/* ensure we instantiate from a IODevice sub-class */
static_assert( std::is_base_of<IODevice, IOType>::value,
               "IOType not derived from IODevice" );
private:
  IOType io_;

public:
  BufferedIO_O( IOType && io )
    : BufferedIO( io_ )
    , io_{ std::move( io ) }
  {}

  BufferedIO_O( BufferedIO_O && other )
    : BufferedIO( io_ )
    , io_{ std::move( other.io_ ) }
  {}

  IOType & io() noexcept { return io_; }
};

#endif /* BUFFERED_IO_HH */
