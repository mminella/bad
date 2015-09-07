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
  static constexpr uint64_t BLOCK = Knobs::IO_BLOCK;
  static constexpr uint64_t ALIGNMENT = 4096;

  using block_ptr = std::pair<const char *, uint64_t>;

private:
  IODevice & io_;
  char * buf_;
  uint64_t bufSize_;
  Channel<block_ptr> blocks_;
  Channel<uint64_t> start_;
  std::thread reader_;
  uint64_t readPass_;
  std::function<void(void)> io_cb_;
  uint64_t id_;

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
      std::cout << "circular_read, " << id_ << ", " << ++readPass_ << ", "
        << nbytes << ", " << timestamp<ms>() << ", start" << std::endl;

      auto ts = time_now();
      tdiff_t tt = 0;
      char * wptr_ = buf_;
      uint64_t rbytes = 0;
      while ( true ) {
        auto t0 = time_now();
        uint64_t n = io_.read( wptr_, BLOCK );
        tt += time_diff<ms>( t0 );

        if ( n > 0 ) {
          rbytes += n;
          blocks_.send( {wptr_, n} );
          wptr_ += BLOCK;
          if ( wptr_ == buf_ + bufSize_ ) {
            wptr_ = buf_;
          }
        }

        if ( rbytes >= nbytes ) {
          io_cb_();
          blocks_.send( { nullptr, 0} ); // indicate EOF
          break;
        }
      }
      auto te = time_diff<ms>( ts );
      std::cout << "circular_read, " << id_ << ", " << readPass_
        << ", " << tt << std::endl;
      std::cout << "circular_read (blocked), " << id_ << ", " << readPass_
        << ", " << te << std::endl;
    }
  }

public:
  CircularIO( IODevice & io, uint64_t blocks, uint64_t id = 0 )
    : io_{io}
    , buf_{nullptr}
    , bufSize_{blocks * BLOCK}
    , blocks_{blocks-2}
    , start_{0}
    , reader_{std::thread( &CircularIO::read_loop,  this )}
    , readPass_{0}
    , io_cb_{[]() {}}
    , id_{id}
  {
    if ( blocks < 3 ) {
      throw new std::runtime_error( "need at least three blocks" );
    }
    posix_memalign( (void **) &buf_, ALIGNMENT, bufSize_ );
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

  void set_io_drained_cb( std::function<void(void)> f )
  {
    io_cb_ = f;
  }
};

#endif /* CIRCULAR_IO_HH */
