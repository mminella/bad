#ifndef CIRCULAR_IO_REC_HH
#define CIRCULAR_IO_REC_HH

#include <cstring>
#include <system_error>

#include "circular_io.hh"

/**
 * Abstracts the block interface of CircularIO into a record interface. Deals
 * with issue of records crossing block boundaries.
 */
template<size_t rec_size>
class CircularIORec : public CircularIO
{
private:
  char rec_[rec_size];
  const char * bend_;
  const char * pos_;

public:
  CircularIORec( IODevice & io )
    : CircularIO( io )
    , bend_{nullptr}
    , pos_{nullptr}
  {};

  /* no copy or move */
  CircularIORec( const CircularIORec & r ) = delete;
  CircularIORec & operator=( const CircularIORec & r ) = delete;
  CircularIORec( CircularIORec && r ) = delete;
  CircularIORec & operator=( CircularIORec && r ) = delete;

  const char * next_record( void )
  {
    // either haven't got first block OR exactly consumed current block
    if ( pos_ == nullptr || pos_ == bend_) {
      auto blk = next_block();
      if ( blk.first == nullptr ) {
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
      memcpy( rec_, pos_, partial );

      auto blk = next_block();
      if ( blk.first == nullptr ) {
        throw std::runtime_error( "corrupt record, wasn't enough data" );
      }
      pos_ = blk.first;
      bend_ = blk.first + blk.second;

      memcpy( rec_ + partial, pos_, rec_size - partial );
      pos_ += rec_size - partial;
      return rec_;
    }
  }
};

#endif /* CIRCULAR_IO_REC_HH */
