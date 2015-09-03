#ifndef CIRCULAR_IO_HH
#define CIRCULAR_IO_HH

#include <cstdlib>
#include <iostream>
#include <memory>
#include <system_error>
#include <thread>

#include "tune_knobs.hh"

#include "channel.hh"
#include "file.hh"
#include "timestamp.hh"

/**
 * Read from an IODevice in a seperate thread in a block-wise fashion, using a
 * channel for notifying when new blocks are ready.
 */
class CircularIO
{
public:
  static constexpr size_t BLOCK = Knobs::IO_BLOCK;
  static constexpr size_t NBLOCKS = Knobs::IO_NBLOCKS;
  static constexpr size_t BUFFER_SIZE = NBLOCKS * BLOCK;
  static constexpr size_t ALIGNMENT = 4096;

  // Need at least three blocks, one for queue, one for reader and one for
  // processing.
  static_assert( NBLOCKS >= 3, "CircularIO requires at least 3 blocks");

  using block_ptr = std::pair<const char *, size_t>;

private:
  IODevice & io_;
  char * buf_;
  Channel<block_ptr> blocks_;
  Channel<uint64_t> start_;
  std::thread reader_;
  uint64_t readPass_;

  /* continually read from the device, taking commands over a channel */
  void read_loop( void )
  {
    while ( true ) {
      uint64_t nbytes = 0;
      try {
        nbytes = start_.recv();
      } catch ( const std::exception & e ) {
          return;
      }

      if ( nbytes == 0 ) {
        continue;
      }
      std::cout << "circular_read, " << ++readPass_ << ", "
        << timestamp<ms>() << ", start" << std::endl;

      auto ts = time_now();
      tdiff_t tt = 0;
      char * wptr_ = buf_;
      uint64_t rbytes = 0;
      while ( true ) {
        auto t0 = time_now();
        size_t n = io_.read( wptr_, BLOCK );
        tt += time_diff<ms>( t0 );

        if ( n > 0 ) {
          rbytes += n;
          blocks_.send( {wptr_, n} );
          wptr_ += BLOCK;
          if ( wptr_ == buf_ + BUFFER_SIZE ) {
            wptr_ = buf_;
          }
        }

        if ( rbytes >= nbytes ) {
          blocks_.send( { nullptr, 0} ); // indicate EOF
          break;
        }
      }
      auto te = time_diff<ms>( ts );
      std::cout << "circular_read, " << readPass_
        << ", " << tt << std::endl;
      std::cout << "circular_read (blocked), " << readPass_
        << ", " << te << std::endl;
    }
  }

public:
  CircularIO( IODevice & io )
    : io_{io}
    , buf_{nullptr}
    , blocks_{NBLOCKS-2}
    , start_{0}
    , reader_{std::thread( &CircularIO::read_loop,  this )}
    , readPass_{0}
  {
    posix_memalign( (void **) &buf_, ALIGNMENT, BUFFER_SIZE );
  };

  /* no copy */
  CircularIO( const CircularIO & r ) = delete;
  CircularIO & operator=( const CircularIO & r ) = delete;

  /* no move */
  CircularIO( CircularIO && r ) = delete;
  CircularIO & operator=( CircularIO && r ) = delete;

  ~CircularIO( void )
  {
    free( buf_ );
    start_.close();
    if ( reader_.joinable() ) { reader_.join(); }
  }

  /* start reading nbytes from the io device (separate thread) */
  void start_read( uint64_t nbytes )
  {
    start_.send( nbytes );
  }

  /* grab next available block of data */
  block_ptr next_block( void )
  {
    return blocks_.recv();
  }
};

#endif /* CIRCULAR_IO_HH */
