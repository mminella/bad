#ifndef CIRCULAR_IO_REC_HH
#define CIRCULAR_IO_REC_HH

#include <cstring>
#include <system_error>

#include "tune_knobs.hh"

#include "circular_io.hh"
#include "sync_print.hh"

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
  size_t recs_;
  size_t rrbytes_;

public:
  CircularIORec( IODevice & io, size_t blocks, int id = 0 )
    : CircularIO( io, blocks, id )
    , bend_{nullptr}
    , pos_{nullptr}
    , recs_{0}
    , rrbytes_{0}
  {};

  /* no copy or move */
  CircularIORec( const CircularIORec & r ) = delete;
  CircularIORec & operator=( const CircularIORec & r ) = delete;
  CircularIORec( CircularIORec && r ) = delete;
  CircularIORec & operator=( CircularIORec && r ) = delete;

  void reset_rec_count( void ) noexcept
  {
    print( "records", 0 );
    recs_ = 0;
  }

  size_t rec_count( void ) const noexcept { return recs_; }

  const char * next_record( void )
  {
    // either haven't got first block OR exactly consumed current block
    if ( pos_ == nullptr || pos_ == bend_) {
      auto blk = next_block();
      print( "cr-recv", (void *) blk.first, blk.second );
      if ( blk.first == nullptr ) {
        return nullptr;
      }
      pos_ = blk.first;
      bend_ = blk.first + blk.second;
      rrbytes_ += blk.second;
    }

    recs_++;
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
      print( "cr-recv", (void *) blk.first, blk.second );
      if ( blk.first == nullptr ) {
        print( "next_record", "fail", partial, blk.second, recs_, rrbytes_ );
        throw std::runtime_error( std::to_string( id() )
          + ": corrupt record, wasn't enough data" );
      }
      pos_ = blk.first;
      bend_ = blk.first + blk.second;
      rrbytes_ += blk.second;

      memcpy( rec_ + partial, pos_, rec_size - partial );
      pos_ += rec_size - partial;
      return rec_;
    }
  }
};

#endif /* CIRCULAR_IO_REC_HH */
