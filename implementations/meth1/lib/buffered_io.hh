#ifndef BUFFERED_IO_HH
#define BUFFERED_IO_HH

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "tune_knobs.hh"

#include "io_device.hh"

/* Wrapper around an IODevice to buffer reads and/or writes. */
class BufferedIO : public IODevice
{
private:
  /* buffers */
  std::unique_ptr<char[]> rbuf_;
  std::unique_ptr<char[]> wbuf_;
  size_t rstart_ = 0, rend_ = 0;
  size_t wstart_ = 0, wend_ = 0;

  /* base buffer read method */
  std::pair<const char *, size_t> rread_buf( size_t limit, bool read_all );

  /* implement read + write */
  size_t rread( char * buf, size_t limit ) override;
  std::string rread( size_t limit = MAX_READ ) override;
  size_t wwrite( const char * buf, size_t nbytes ) override;

protected:
  /* underlying file descriptor */
  IODevice * io_;

  /* io device state */
  bool get_eof( void ) const noexcept override { return io_->eof(); }
  void set_eof( void ) noexcept override {}
  void reset_eof( void ) noexcept override {}

public:
  static constexpr size_t BUFFER_SIZE = Knobs::IO_BUFFER;

  /* constructor */
  BufferedIO( IODevice & io )
    : rbuf_{new char[BUFFER_SIZE]}
    , wbuf_{new char[BUFFER_SIZE]}
    , io_{&io}
  {
  }
  
  /* no copy */
  BufferedIO( const BufferedIO & ) = delete;
  BufferedIO & operator=( const BufferedIO & ) = delete;

  /* allow move */
  BufferedIO( BufferedIO && other )
    : rbuf_{std::move( other.rbuf_ )}
    , wbuf_{std::move( other.wbuf_ )}
    , rstart_{other.rstart_}
    , rend_{other.rend_}
    , wstart_{other.wstart_}
    , wend_{other.wend_}
    , io_{other.io_}
  {
  }

  BufferedIO & operator=( BufferedIO && other )
  {
    if ( this != &other ) {
      rbuf_ = std::move( other.rbuf_ );
      wbuf_ = std::move( other.wbuf_ );
      rstart_ = other.rstart_;
      rend_ = other.rend_;
      wstart_ = other.wstart_;
      wend_ = other.wend_;
      io_ = other.io_;
    }
    return *this;
  }

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
  IOType owned_io_;

public:
  BufferedIO_O( IOType && io )
    : BufferedIO{owned_io_}
    , owned_io_{std::move( io )}
  {}

  /* no copy */
  BufferedIO_O( const BufferedIO_O & other ) = delete;
  BufferedIO_O & operator=( BufferedIO_O & other ) = delete;

  /* allow move */
  BufferedIO_O( BufferedIO_O && other )
    : BufferedIO{std::move( other )}
    , owned_io_{std::move( other.owned_io_ )}
  {
    // need to correct the pointer since we moved the IO device.
    io_ = &owned_io_;
  }

  BufferedIO_O & operator=( BufferedIO_O && other )
  {
    if ( this != &other ) {
      BufferedIO::operator=( std::move( other ) );
      io_ = std::move( other.io_ );
    }
    return *this;
  }

  IOType & io() noexcept { return owned_io_; }
};

#endif /* BUFFERED_IO_HH */
