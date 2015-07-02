#ifndef BUFFERED_IO_HH
#define BUFFERED_IO_HH

#include <unistd.h>

#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>

#include "exception.hh"

class FileDescriptor;

/* Wrapper around a FileDescriptor subclass to buffer reads and/or writes. */
template <typename IOType>
class BufferedIO
{
public:
  /* ensure we instantiate from a FileDescriptor sub-class */
  static_assert(std::is_base_of<FileDescriptor, IOType>::value,
    "IOType not derived from FileDescriptor");

  using iterator_type = std::string::const_iterator;

  const static size_t BUFFER_SIZE = 4096;

private:
  /* buffers */
  std::unique_ptr<char> rbuf_;
  std::unique_ptr<char> wbuf_;
  const size_t rsize_ = 0, wsize_ = 0;
  size_t rstart_ = 0, rend_ = 0;
  size_t wstart_ = 0, wend_ = 0;

  /* underlying device */
  IOType io_;

  ssize_t read_raw( char * buf, size_t count );
  ssize_t write_raw( const char * buf, size_t count );
  ssize_t wwrite( const char * buf, size_t count );

public: /* BufferedIO API */

  /* BufferedIO takes ownership of the file descriptor. You can access the
   * underlying type through `iodevice`. */
  BufferedIO( IOType && io, size_t rbuf = BUFFER_SIZE, size_t wbuf = BUFFER_SIZE )
    : rbuf_{ new char[BUFFER_SIZE]() }
    , wbuf_{ wbuf ? new char[BUFFER_SIZE]() : nullptr }
    , rsize_{ rbuf }
    , wsize_{ wbuf }
    , io_{ std::move( io ) }
  {
  }

  /* move */
  BufferedIO( BufferedIO && other ) noexcept
    : rbuf_{ std::move( other.rbuf_ ) }
    , wbuf_{ std::move( other.wbuf_ ) }
    , rsize_{ other.rsize_ }
    , wsize_{other.wsize_}
    , rstart_{other.rstart_}
    , rend_{other.rend_}
    , wstart_{other.wstart_}
    , wend_{other.wend_}
    , io_{ std::move( other.io_ ) }
  {
  }

  BufferedIO & operator=( BufferedIO && other ) noexcept
  {
    if ( this != &other ) {
      rbuf_ = std::move( other.rbuf_ );
      wbuf_ = std::move( other.wbuf_ );
      rsize_ = other.rsize_;
      wsize_ = other.wsize_;
      rstart_ = other.rstart_;
      rend_ = other.rend_;
      wstart_ = other.wstart_;
      wend_ = other.wend_;
      io_ = std::move( other.io_ );
    }
    return *this;
  }

  /* accessors */
  IOType & iodevice( void ) noexcept { return io_; }
  const int & fd_num( void ) const noexcept { return io_.fd_num(); }
  const bool & eof( void ) const noexcept { return io_.eof(); }
  unsigned int read_count( void ) const noexcept { return io_.read_count(); }
  unsigned int write_count( void ) const noexcept { return io_.write_count(); }

  /* read method */
  std::tuple<const char *, size_t> buffer_read( size_t limit = BUFFER_SIZE,
                                                bool read_all = false,
                                                size_t read_ahead = BUFFER_SIZE );

  std::string read( size_t limit = BUFFER_SIZE, bool read_all = false,
                    size_t read_ahead = BUFFER_SIZE )
  {
    std::string str;
    if ( read_all and limit > 0 ) {
      str.reserve( limit );
    }

    size_t n;
    const char * cstr;
    do {
      std::tie( cstr, n ) = BufferedIO::buffer_read( limit - str.size(),
                                                     read_all, read_ahead );
      str += std::string( cstr, n );
    } while ( read_all and str.size() < limit );

    return str;
  }

  /* write methods */
  ssize_t write( const char * buffer, size_t count, bool write_all = true )
  {
    size_t n{0};

    do {
      n += wwrite( buffer + n, count - n );
    } while ( write_all and n < count );

    return n;
  }

  iterator_type write( const std::string & buffer, bool write_all = true )
  {
    auto it = buffer.begin();

    do {
      it += wwrite( &*it, buffer.end() - it );
    } while ( write_all and it != buffer.end() );

    return it;
  }
  
  /* flush */
  ssize_t flush( bool flush_all = true );

  /* destructor */
  ~BufferedIO() { flush( true ); }
};

/* raw (direct) read */
template <typename IOType>
ssize_t BufferedIO<IOType>::read_raw( char * buf, size_t count )
{
  ssize_t n = SystemCall( "read", ::read( io_.fd_num(), buf, count ) );
  if ( n == 0 ) {
    io_.set_eof();
  }
  io_.register_read();
  return n;
}

/* buffered read */
template <typename IOType>
std::tuple<const char *, size_t>
BufferedIO<IOType>::buffer_read( size_t limit, bool read_all, size_t read_ahead )
{
  if ( limit == 0 ) {
    throw std::runtime_error( "asked to read 0" );
  }

  char * buf = rbuf_.get();

  /* can't return large segments than our buffer */
  limit = std::min( limit, rsize_ );

  /* direct read if not buffered */
  if ( rsize_ == 0 ) {
    ssize_t n = read_raw( buf, limit );
    return std::make_tuple( buf, n );
  }

  /* can fill (at least partially) from cache */
  if ( rstart_ < rend_ ) {
    if ( !read_all ) { limit = std::min( limit, rend_ - rstart_ ); }

    if ( limit <= rend_ - rstart_ ) {
      /* can completely fill from cache */
      const char * str = buf + rstart_;
      rstart_ += limit;
      return std::make_tuple( str, limit );
    } else {
      /* move cached to start of buffer */
      memmove( buf, buf + rstart_, rend_ - rstart_ );
      rend_ = rend_ - rstart_;
      rstart_ = 0;
    }
  } else {
    rstart_ = rend_ = 0;
  }

  /* cache empty, refill */
  read_ahead = std::min( read_ahead - rend_, rsize_ - rend_ );
  ssize_t n = read_raw( buf + rend_, read_ahead );
  rend_ += n;

  /* return from cache */
  limit = std::min( static_cast<size_t>( n ), limit );
  rstart_ += limit;
  return std::make_tuple( buf, limit );
}

/* raw (direct) write */
template <typename IOType>
ssize_t BufferedIO<IOType>::write_raw( const char * buf, size_t count)
{
  ssize_t n = SystemCall( "write", ::write( io_.fd_num(), buf, count ) );
  if ( n == 0 ) {
    throw std::runtime_error( "write returned 0" );
  }
  io_.register_write();
  return n;
}

/* buffered write */
template <typename IOType>
ssize_t BufferedIO<IOType>::wwrite( const char * buf, size_t count)
{
  if ( count == 0 ) {
    throw std::runtime_error( "nothing to write" );
  }

  /* direct write since buffer disabled */
  if ( wsize_ == 0 ) {
    return write_raw( buf, count );
  }

  /* copy to available buffer space */
  size_t limit = std::min( count, wsize_ - wend_ );
  if ( limit > 0 ) {
    std::memcpy( wbuf_.get() + wend_, buf, limit );
    wend_ += limit;
  }

  /* buffer full so flush */
  if ( limit != count ) {
    flush( false );
  }

  return limit;
}

/* flush */
template <typename IOType>
ssize_t BufferedIO<IOType>::flush( bool flush_all )
{
  if ( wstart_ == wend_ ) {
    return 0;
  }

  ssize_t n {0};

  do {
    n = write_raw( wbuf_.get() + wstart_, wend_ - wstart_ );
    wstart_ += n;
  } while ( flush_all and wstart_ != wend_ );

  if ( wstart_ == wend_ ) {
    wstart_ = wend_ = 0;
  }

  return n;
}

#endif /* BUFFERED_IO_HH */
