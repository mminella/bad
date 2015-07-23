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
#include "io_device.hh"

/*
 * Simply reads a file in blocks in another thread, notifying a caller through
 * a channel as each block is ready.
 */
class OverlappedIO
{
public:
  static constexpr size_t BLOCK = 4096 * 10; // 40KB
  static constexpr size_t NBLOCKS = 10;
  static constexpr size_t BUF_SIZE = NBLOCKS * BLOCK;

  // Need at least two blocks, one for reader and one for processing.
  static_assert( NBLOCKS > 1, "OverlappedIO requires at least 2 blocks");

  using block_ptr = std::pair<const char *, size_t>;

private:
  IODevice & io_;
  char * buf_ = new char[BUF_SIZE];
  Channel<block_ptr> chn_;
  std::thread reader_;

  void read_file( void )
  {
    char * wptr_ = buf_;
    while ( true ) {
      size_t n = io_.read_all( wptr_, BLOCK );

      if ( n > 0 ) {
        chn_.send( {wptr_, n} );
        wptr_ += BLOCK;
        if ( wptr_ == buf_ + BUF_SIZE ) {
          wptr_ = buf_;
        }
      }

      if ( io_.eof() ) {
        chn_.send( { nullptr, 0} ); // indicate EOF
        break;
      }
    }
  }

public:
  OverlappedIO( IODevice & io ) : io_{io}, chn_{NBLOCKS-1}, reader_{} {};

  OverlappedIO( const OverlappedIO & r ) = delete;
  OverlappedIO( OverlappedIO && r ) = delete;
  OverlappedIO & operator=( const OverlappedIO & r ) = delete;
  OverlappedIO & operator=( OverlappedIO && r ) = delete;

  ~OverlappedIO( void )
  {
    delete buf_;
    if ( reader_.joinable() ) { reader_.join(); }
  }

  void start( void )
  {
    reader_ = std::thread( &OverlappedIO::read_file,  this );
  }

  block_ptr next_block( void )
  {
    return chn_.recv();
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
  const char * block_;
  const char * bend_;
  const char * pos_;
  bool eof_;

public:
  OverlappedRecordIO( IODevice & io )
    : io_{io}, block_{nullptr}, bend_{nullptr}, pos_{nullptr}, eof_{false}
  {};

  OverlappedRecordIO( const OverlappedRecordIO & r ) = delete;
  OverlappedRecordIO( OverlappedRecordIO && r ) = delete;
  OverlappedRecordIO & operator=( const OverlappedRecordIO & r ) = delete;
  OverlappedRecordIO & operator=( OverlappedRecordIO && r ) = delete;

  void start( void ) { io_.start(); }

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
      pos_ = block_ = blk.first;
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
      pos_ = block_ = blk.first;
      bend_ = blk.first + blk.second;

      memcpy( rec + partial, pos_, rec_size - partial );
      pos_ += rec_size - partial;
      return rec;
    }
  }
};

#endif /* OVERLAPPED_IO_HH */
