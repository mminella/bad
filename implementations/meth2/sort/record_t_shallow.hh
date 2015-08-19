#ifndef RECORD_T_SHALLOW_HH
#define RECORD_T_SHALLOW_HH

#include <cstdint>
#include <cstring>
#include <iostream>
#include <utility>

#include "io_device.hh"

#include "alloc.hh"
#include "record_common.hh"

/**
 * Version of Record that only performs a shallow copy on copy construction or
 * copy assignment. THIS IS DANGEROUS! It's very easy to end up with two
 * RecordS's that both think they own the same value memory, leading to
 * double-free's or use-after-free.
 *
 * However, we provide it as when used correctly it can be useful for some
 * performance sensitive situations.
 */
class RecordS
{
public:
  uint64_t loc_ = 0;
  uint8_t * val_ = nullptr;
  uint8_t key_[Rec::KEY_LEN];

  void copy( const uint8_t* r, uint64_t i ) noexcept
  {
    loc_ = i;
    if ( val_ == nullptr ) { val_ = Rec::alloc_val(); }
    memcpy( val_, r + Rec::KEY_LEN, Rec::VAL_LEN );
    memcpy( key_, r, Rec::KEY_LEN );
  }

  void copy( const RecordS & r ) noexcept
  {
    loc_ = r.loc_;
    if ( val_ == nullptr ) { val_ = Rec::alloc_val(); }
    memcpy( val_, r.val_, Rec::VAL_LEN );
    memcpy( key_, r.key_, Rec::KEY_LEN );
  }

  void copy( RecordS && r ) noexcept
  {
    loc_ = r.loc_;
    val_ = r.val_;
    memcpy( key_, r.key_, Rec::KEY_LEN );
  }

  RecordS( void ) noexcept {}

  /* Construct a min or max record. */
  RecordS( Rec::limit_t lim ) noexcept
    : val_{Rec::alloc_val()}
  {
    if ( lim == Rec::MAX ) {
      memset( key_, 0xFF, Rec::KEY_LEN );
    } else {
      memset( key_, 0x00, Rec::KEY_LEN );
    }
  }

  /* Construct from c string read from disk */
  RecordS( const uint8_t * s, uint64_t loc = 0 ) { copy( s, loc ); }
  RecordS( const char * s, uint64_t loc = 0 ) { copy( (uint8_t *) s, loc ); }

  RecordS( const RecordS & other )
    : loc_{other.loc_}
  {
    val_ = other.val_;
    memcpy( key_, other.key_, Rec::KEY_LEN );
  }

  RecordS & operator=( const RecordS & other )
  {
    if ( this != &other ) {
      loc_ = other.loc_;
      val_ = other.val_;
      memcpy( key_, other.key_, Rec::KEY_LEN );
    }
    return *this;
  }

  RecordS( RecordS && other )
    : loc_{other.loc_}
  {
    std::swap( val_, other.val_ );
    memcpy( key_, other.key_, Rec::KEY_LEN );
  }

  RecordS & operator=( RecordS && other )
  {
    if ( this != &other ) {
      loc_ = other.loc_;
      std::swap( val_, other.val_ );
      memcpy( key_, other.key_, Rec::KEY_LEN );
    }
    return *this;
  }

  ~RecordS( void ) { Rec::dealloc_val( val_ ); }

  /* Accessors */
  const uint8_t * key( void ) const noexcept { return key_; }
  const uint8_t * val( void ) const noexcept { return val_; }
  uint64_t loc( void ) const noexcept { return loc_; }

  /* methods for boost::sort */
  const char * data( void ) const noexcept { return (char *) key_; }
  unsigned char operator[]( size_t i ) const noexcept { return key_[i]; }
  size_t size( void ) const noexcept { return Rec::KEY_LEN; }

  /* comparison */
  comp_op( <, Record )
  comp_op( <, RecordS )
  comp_op( <, RecordPtr )
  comp_op( <=, Record )
  comp_op( <=, RecordS )
  comp_op( <=, RecordPtr )
  comp_op( >, Record )
  comp_op( >, RecordS )
  comp_op( >, RecordPtr )

  int compare( const uint8_t * k, uint64_t l ) const noexcept;
  int compare( const Record & b ) const noexcept;
  int compare( const RecordS & b ) const noexcept;
  int compare( const RecordPtr & b ) const noexcept;

  /* Write to IO device */
  void write( IODevice & io, Rec::loc_t locinfo = Rec::NO_LOC ) const
  {
    io.write_all( (char *) key_, Rec::KEY_LEN );
    io.write_all( (char *) val_, Rec::VAL_LEN );
    if ( locinfo == Rec::WITH_LOC ) {
      io.write_all( reinterpret_cast<const char *>( &loc_ ),
                    sizeof( uint64_t ) );
    }
  }
};

std::ostream & operator<<( std::ostream & o, const RecordS & r );

inline void iter_swap ( RecordS * a, RecordS * b ) noexcept
{
  std::swap( *a, *b );
}

#endif /* RECORD_T_SHALLOW_HH */