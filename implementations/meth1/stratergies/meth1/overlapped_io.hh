#ifndef OVERLAPPED_IO_HH
#define OVERLAPPED_IO_HH

/*
 * Defines a simple abstraction for sequentially reading a file while
 * overlapping computation on blocks of the file.
 */

#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <system_error>
#include <thread>
#include <vector>

#include "channel.hh"
#include "exception.hh"
#include "file.hh"
#include "timestamp.hh"

/*
 * Simply reads a file in blocks in another thread, notifying a caller through
 * a channel as each block is ready.
 *
 * This assumes we can generally perform successful reads at BLOCK size, so
 * will only really work well for files.
 */
class OverlappedIO
{
public:
  static constexpr size_t BLOCK = 4096 * 256;
  static constexpr size_t NBLOCKS = 100;
  static constexpr size_t BUF_SIZE = NBLOCKS * BLOCK;
  static constexpr size_t ALIGNMENT = 4096;

  // Need at least three blocks, one for queue, one for reader and one for
  // processing.
  static_assert( NBLOCKS >= 3, "OverlappedIO requires at least 3 blocks");

  using block_ptr = std::pair<const char *, size_t>;

private:
  File & io_;
  char * buf_ = (char *) aligned_alloc( ALIGNMENT, BUF_SIZE );
  Channel<block_ptr> block_chn_;
  Channel<bool> ctrl_chn_;
  std::thread reader_;

  void read_file( void )
  {
    while ( true ) {
      try {
        ctrl_chn_.recv();
      } catch ( const std::exception & e ) {
          return;
      }
      std::cout << "= read start: " << timestamp<ms>() << std::endl;
      io_.rewind();

      char * wptr_ = buf_;
      while ( true ) {
        size_t n = io_.read( wptr_, BLOCK );

        if ( n > 0 ) {
          block_chn_.send( {wptr_, n} );
          wptr_ += BLOCK;
          if ( wptr_ == buf_ + BUF_SIZE ) {
            wptr_ = buf_;
          }
        }

        if ( io_.eof() || n < BLOCK ) {
          block_chn_.send( { nullptr, 0} ); // indicate EOF
          break;
        }
      }
      std::cout << "= read done: " << timestamp<ms>() << std::endl;
    }
  }

public:
  OverlappedIO( File & io )
    : io_{io}, block_chn_{NBLOCKS-2}, ctrl_chn_{0}, reader_{}
  {
    reader_ = std::thread( &OverlappedIO::read_file,  this );
  };

  OverlappedIO( const OverlappedIO & r ) = delete;
  OverlappedIO( OverlappedIO && r ) = delete;
  OverlappedIO & operator=( const OverlappedIO & r ) = delete;
  OverlappedIO & operator=( OverlappedIO && r ) = delete;

  ~OverlappedIO( void )
  {
    ctrl_chn_.close();
    if ( reader_.joinable() ) { reader_.join(); }
    delete buf_;
  }

  void rewind( void )
  {
    ctrl_chn_.send(true);
  }

  block_ptr next_block( void )
  {
    return block_chn_.recv();
  }
};

/**
 * Abstracts the block interface of OverlappedIO into a record interface. Deals
 * with issue of records crossing block boundaries.
 */
template<size_t rec_size>
class OverlappedRecordIO
{
private:
  char rec[rec_size];
  OverlappedIO io_;
  const char * bend_;
  const char * pos_;
  bool eof_;

public:
  OverlappedRecordIO( File & io )
    : io_{io}, bend_{nullptr}, pos_{nullptr}, eof_{false}
  {};

  OverlappedRecordIO( const OverlappedRecordIO & r ) = delete;
  OverlappedRecordIO( OverlappedRecordIO && r ) = delete;
  OverlappedRecordIO & operator=( const OverlappedRecordIO & r ) = delete;
  OverlappedRecordIO & operator=( OverlappedRecordIO && r ) = delete;

  void rewind( void )
  {
    eof_ = false;
    io_.rewind();
  }

  bool eof( void ) const noexcept { return eof_; }

  const char * next_record( void )
  {
    if ( eof_ ) {
      return nullptr;
    }

    // either haven't got first block OR exactly consumed current block
    if ( pos_ == nullptr || pos_ == bend_) {
      auto blk = io_.next_block();
      if ( blk.first == nullptr ) {
        eof_ = true;
        return nullptr;
      }
      pos_ = blk.first;
      bend_ = blk.first + blk.second;
    }

    if ( pos_ + rec_size <= bend_ ) {
      // record within a single block
      const char * p = pos_;
      pos_ += rec_size;
      return p;
    } else {
      // record cross block boundary
      size_t partial = bend_ - pos_;
      memcpy( rec, pos_, partial );

      auto blk = io_.next_block();
      if ( blk.first == nullptr ) {
        throw std::runtime_error( "corrupt record, wasn't enough data" );
      }
      pos_ = blk.first;
      bend_ = blk.first + blk.second;

      memcpy( rec + partial, pos_, rec_size - partial );
      pos_ += rec_size - partial;
      return rec;
    }
  }
};

#endif /* OVERLAPPED_IO_HH */
