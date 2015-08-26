#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

#include "buffered_io.hh"
#include "exception.hh"
#include "io_device.hh"

using namespace std;

/* needed as we need a definition since std::min takes references */
const size_t BufferedIO::BUFFER_SIZE;

/* read, returning a pointer into the internal buffer */
pair<const char *, size_t> BufferedIO::rread_buf( size_t limit, bool read_all )
{
  register_read();

  char * buf = rbuf_.get();

  /* can't return large segments than our buffer */
  limit = min( limit, BUFFER_SIZE );

  /* can fill (at least partially) from cache */
  if ( rstart_ < rend_ ) {
    if ( limit <= rend_ - rstart_ ) {
      /* can completely fill from cache */
      const char * str = buf + rstart_;
      rstart_ += limit;
      return make_pair( str, limit );
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
  do {
    rend_ += io_->read( buf + rend_, BUFFER_SIZE - rend_ );
  } while ( read_all and rend_ < limit and !io_->eof() );

  /* return from cache */
  limit = min( rend_, limit );
  rstart_ += limit;
  return make_pair( buf, limit );
}

pair<const char *, size_t> BufferedIO::read_buf( size_t limit )
{
  if ( limit == 0 ) {
    throw runtime_error( "asked to read 0" );
  }
  return rread_buf( limit, false );
}

pair<const char *, size_t> BufferedIO::read_buf_all( size_t nbytes )
{
  if ( nbytes == 0 ) {
    throw runtime_error( "asked to read 0" );
  } else if ( nbytes > BUFFER_SIZE ) {
    throw runtime_error( "read request larger than buffer!" );
  }
  return rread_buf( nbytes, true );
}

/* overriden base read method */
size_t BufferedIO::rread( char * buf, size_t limit )
{
  auto cstr = rread_buf( limit, false );
  if ( cstr.second > 0 ) {
    memcpy( buf, cstr.first, cstr.second );
  }
  return cstr.second;
}

/* overriden base read method */
string BufferedIO::rread( size_t limit )
{
  auto cstr = rread_buf( limit, false );
  return {cstr.first, cstr.second};
}

/* overriden base write method */
size_t BufferedIO::wwrite( const char * buf, size_t nbytes )
{
  /* copy to available buffer space */
  size_t limit = min( nbytes, BUFFER_SIZE - wend_ );
  if ( limit > 0 ) {
    memcpy( wbuf_.get() + wend_, buf, limit );
    wend_ += limit;
  }

  /* buffer full so flush */
  if ( wend_ == BUFFER_SIZE ) {
    flush( false );
  }

  return limit;
}

/* flush */
size_t BufferedIO::flush( bool flush_all )
{
  if ( wstart_ == wend_ ) {
    return 0;
  }

  size_t n = 0;
  if ( flush_all ) {
    n = io_->write_all( wbuf_.get() + wstart_, wend_ - wstart_ );
  } else {
    n = io_->write( wbuf_.get() + wstart_, wend_ - wstart_ );
  }
  wstart_ += n;

  if ( wstart_ == wend_ ) {
    wstart_ = wend_ = 0;
  }

  return n;
}
