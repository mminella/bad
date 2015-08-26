#ifndef OVERLAPPED_IO_HH
#define OVERLAPPED_IO_HH

#include <cstdlib>
#include <iostream>
#include <memory>
#include <system_error>
#include <thread>

#include "channel.hh"
#include "file.hh"
#include "timestamp.hh"

/**
 * Read a file in a seperate thread in a block-wise fashion, using a channel
 * for notifying when new blocks are ready. Specific to files for now.
 */
class OverlappedIO
{
public:
  static constexpr size_t BLOCK = 4096 * 256 * 10; // 10MB
  static constexpr size_t NBLOCKS = 100;           // 1GB
  static constexpr size_t BUF_SIZE = NBLOCKS * BLOCK;
  static constexpr size_t ALIGNMENT = 4096;

  // Need at least three blocks, one for queue, one for reader and one for
  // processing.
  static_assert( NBLOCKS >= 3, "OverlappedIO requires at least 3 blocks");

  using block_ptr = std::pair<const char *, size_t>;

private:
  File * io_;
  std::unique_ptr<char> buf_;
  Channel<block_ptr> blocks_;
  Channel<bool> start_;
  std::thread reader_;

  void read_file( void )
  {
    static uint64_t pass = 0;
    while ( true ) {
      try {
        start_.recv();
      } catch ( const std::exception & e ) {
          return;
      }
      std::cout << "read_file, " << ++pass << ", " << timestamp<ms>()
        << ", start" << std::endl;
      io_->rewind();

      auto ts = time_now();
      tdiff_t tt = 0;
      char * buf = buf_.get();
      char * wptr_ = buf;
      while ( true ) {
        auto t0 = time_now();
        size_t n = io_->read( wptr_, BLOCK );
        tt += time_diff<ms>( t0 );

        if ( n > 0 ) {
          blocks_.send( {wptr_, n} );
          wptr_ += BLOCK;
          if ( wptr_ == buf + BUF_SIZE ) {
            wptr_ = buf;
          }
        }

        if ( io_->eof() || n < BLOCK ) {
          blocks_.send( { nullptr, 0} ); // indicate EOF
          break;
        }
      }
      auto te = time_diff<ms>( ts );
      std::cout << "read, " << pass << ", " << tt << std::endl;
      std::cout << "read (blocked), " << pass << ", " << te << std::endl;
    }
  }

public:
  OverlappedIO( File & io )
    : io_{&io}
    , buf_{}
    , blocks_{NBLOCKS-2}
    , start_{0}
    , reader_{std::thread( &OverlappedIO::read_file,  this )}
  {
    char * b;
    posix_memalign( (void **) &b, ALIGNMENT, BUF_SIZE );
    buf_.reset( b );
  };

  /* no copy */
  OverlappedIO( const OverlappedIO & r ) = delete;
  OverlappedIO & operator=( const OverlappedIO & r ) = delete;

  /* allow move */
  OverlappedIO( OverlappedIO && r )
    : io_{r.io_}
    , buf_{std::move( r.buf_ )}
    , blocks_{std::move( r.blocks_ )}
    , start_{std::move( r.start_ )}
    , reader_{std::move( r.reader_ )}
  {
  }

  OverlappedIO & operator=( OverlappedIO && r )
  {
    if ( this != &r ) {
      io_ = r.io_;
      buf_ = std::move( r.buf_ );
      blocks_ = std::move( r.blocks_ );
      start_ = std::move( r.start_ );
      reader_ = std::move( r.reader_ );
    }
    return *this;
  }

  ~OverlappedIO( void )
  {
    start_.close();
    if ( reader_.joinable() ) { reader_.join(); }
  }

  void rewind( void )
  {
    start_.send(true);
  }

  block_ptr next_block( void )
  {
    return blocks_.recv();
  }
};

#endif /* OVERLAPPED_IO_HH */
