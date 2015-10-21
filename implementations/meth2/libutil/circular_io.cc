#include "circular_io.hh"

using namespace std;

constexpr size_t CircularIO::BLOCK;

/* construct a CircularIO */
CircularIO::CircularIO( IODevice & io, size_t blocks, int id )
  : io_{io}
  , buf_{nullptr}
  , bufSize_{blocks * BLOCK}
  , blocks_{blocks-2}
  , start_{0}
  , reader_{}
  , io_cb_{[]() {}}
  , readPass_{0}
  , id_{id}
{
  if ( blocks < 3 ) {
    throw new runtime_error( "need at least three blocks" );
  }
  int r = posix_memalign( (void **) &buf_, ALIGNMENT, bufSize_ );
  if ( r != 0 ) {
    throw new bad_alloc();
  }
  reader_ = thread( &CircularIO::read_loop,  this );
}

/* destructor */
CircularIO::~CircularIO( void )
{
  start_.close();
  blocks_.close();
  if ( reader_.joinable() ) { reader_.join(); }
  free( buf_ );
}

/* start reading nbytes from the io device (separate thread) */
void CircularIO::start_read( size_t nbytes )
{
  start_.send( nbytes );
}

/* grab next available block of data */
CircularIO::block_ptr CircularIO::next_block( void )
{
  return blocks_.recv();
}

/* set the callback to invoke when IO has been dreained */
void CircularIO::set_io_drained_cb( function<void(void)> f )
{
  io_cb_ = f;
}

/* return id of back IODevice */
int CircularIO::id( void ) const noexcept
{
  return id_;
}

/* continually read from the device, taking commands over a channel */
void CircularIO::read_loop( void )
{
  try {
    while ( true ) {
      size_t nbytes = start_.recv();
      if ( nbytes == 0 ) {
        continue;
      }
      //print( "circular-read-start", id_, ++readPass_, nbytes, timestamp<ms>() 
      //);

      auto t0 = time_now();
      tdiff_t tread = 0;
      char * wptr_ = buf_;
      size_t rbytes = 0;
      while ( true ) {
        auto t0 = time_now();
        // should only issue disk block size reads when using O_DIRECT
        size_t blkSize = io_.is_odirect()
          ? BLOCK : min( BLOCK, nbytes - rbytes );
        size_t n = io_.read( wptr_, blkSize );
        tread += time_diff<ms>( t0 );

        if ( n > 0 ) {
          rbytes += n;
          blocks_.send( {wptr_, n} );
          wptr_ += BLOCK;
          if ( wptr_ == buf_ + bufSize_ ) {
            wptr_ = buf_;
          }
        }

        if ( rbytes == nbytes ) {
          io_cb_();
          blocks_.send( { nullptr, 0} ); // indicate EOF
          break;
        } else if ( rbytes > nbytes ) {
          throw runtime_error( "read more bytes than should have been" \
            " available ( " + to_string( rbytes ) + " vs " +
            to_string( nbytes ) + " )" );
        }
      }
      auto tblocked = time_diff<ms>( t0 );
      //print( "circular-read-total", id_, readPass_, rbytes, tread, tblocked 
      //);
    }
  } catch ( const Channel<size_t>::closed_error & e  ) {
    // Allow closing channel to kill thread
    return;
  } catch ( const Channel<block_ptr>::closed_error & e  ) {
    return;
  }
}
