#ifndef OVERLAPPED_REC_IO_HH
#define OVERLAPPED_REC_IO_HH

#include <cstring>
#include <system_error>

#include "overlapped_io.hh"

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

#endif /* OVERLAPPED_REC_IO_HH */
