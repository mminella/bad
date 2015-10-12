#ifndef CIRCULAR_IO_VAR_HH
#define CIRCULAR_IO_VAR_HH

#include <cstring>
#include <system_error>

#include "tune_knobs.hh"

#include "circular_io.hh"
#include "sync_print.hh"

/**
 * Abstracts the block interface of CircularIO into a record interface. Deals
 * with issue of records crossing block boundaries. Assumes the first byte of
 * the record encodes the length.
 */
class CircularIOVar : public CircularIO
{
public:
  using rec_t = std::pair<const char *, size_t>;

private:
  char rec_[256];
  const char * bend_;
  const char * pos_;
  size_t recs_;
  size_t rrbytes_;
  size_t rec_size_;

public:
  CircularIOVar( IODevice & io, size_t blocks, int id = 0 )
    : CircularIO{io, blocks, id}
    , bend_{nullptr}
    , pos_{nullptr}
    , recs_{0}
    , rrbytes_{0}
    , rec_size_{0}
  {};

  /* no copy or move */
  CircularIOVar( const CircularIOVar & r ) = delete;
  CircularIOVar & operator=( const CircularIOVar & r ) = delete;
  CircularIOVar( CircularIOVar && r ) = delete;
  CircularIOVar & operator=( CircularIOVar && r ) = delete;

  void reset_rec_count( void ) noexcept
  {
    recs_ = 0;
  }

  size_t rec_count( void ) const noexcept { return recs_; }

  rec_t next_record( void )
  {
    while ( true ) {
      // either haven't got first block OR exactly consumed current block
      if ( pos_ == nullptr || pos_ >= bend_) {
        auto blk = next_block();
        if ( blk.first == nullptr ) {
          return std::make_pair( nullptr, 0 );
        }
        pos_ = blk.first;
        bend_ = blk.first + blk.second;
        rrbytes_ += blk.second;
      }

      while ( rec_size_ == 0 and pos_ < bend_ ) {
        rec_size_ = (uint8_t) pos_[0];
        pos_++;
      }

      recs_++;
      if ( pos_ + rec_size_ <= bend_ ) {
        // record within a single block
        const char * p = pos_;
        pos_ += rec_size_;
        size_t rsize = rec_size_;
        rec_size_ = 0;
        return std::make_pair( p, rsize );
      } else if ( pos_ < bend_ ) {
        // record cross block boundary
        size_t partial = bend_ - pos_;
        memcpy( rec_, pos_, partial );

        auto blk = next_block();
        if ( blk.first == nullptr ) {
          // since variable length records, final one may be corrupt.
          return std::make_pair( nullptr, 0 );
        }
        pos_ = blk.first;
        bend_ = blk.first + blk.second;
        rrbytes_ += blk.second;

        memcpy( rec_ + partial, pos_, rec_size_ - partial );
        pos_ += rec_size_ - partial;
        size_t rsize = rec_size_;
        rec_size_ = 0;
        return std::make_pair( rec_, rsize );
      }
    }
  }
};

#endif /* CIRCULAR_IO_VAR_HH */
