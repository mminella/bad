#ifndef RECORD_T_HH
#define RECORD_T_HH

#include <cstdint>
#include <cstring>
#include <iostream>
#include <utility>

#include "io_device.hh"

#include "alloc.hh"
#include "record_common.hh"
#include "record_ptr.hh"
#include "record_t_shallow.hh"

/**
 * Represent a record in the sort benchmark.
 */
class Record
{
private:
#if WITHLOC == 1
  uint64_t loc_ = 0;
#endif
  uint8_t * val_ = nullptr;
  uint8_t key_[Rec::KEY_LEN];

public:
  void copy( const uint8_t * k, const uint8_t * v, uint64_t i ) noexcept
  {
#if WITHLOC == 1
    loc_ = i;
#else
    (void) i;
#endif
    if ( val_ == nullptr ) { val_ = Rec::alloc_val(); }
    memcpy( val_, v, Rec::VAL_LEN );
    memcpy( key_, k, Rec::KEY_LEN );
  }

  void copy( const uint8_t * r, uint64_t i ) noexcept
  {
    copy( r, r + Rec::KEY_LEN, i );
  }

  void copy( const Record & r ) noexcept
  {
    copy( r.key(), r.val(), r.loc() );
  }

  void copy( const RecordS & r ) noexcept
  {
    copy( r.key(), r.val(), r.loc() );
  }

  void copy( const RecordPtr & r ) noexcept
  {
    copy( r.key(), r.val(), r.loc() );
  }

  Record( void ) noexcept {}

  /* Construct a min or max record. */
  Record( Rec::limit_t lim ) noexcept
    : val_{Rec::alloc_val()}
  {
    if ( lim == Rec::MAX ) {
      memset( key_, 0xFF, Rec::KEY_LEN );
    } else {
      memset( key_, 0x00, Rec::KEY_LEN );
    }
  }

  /* Construct from c string read from disk */
  Record( const uint8_t * s, uint64_t loc = 0 ) { copy( s, loc ); }
  Record( const char * s, uint64_t loc = 0 ) { copy( (uint8_t *) s, loc ); }

  Record( const Record & other )
#if WITHLOC == 1
    : loc_{other.loc_}
    , val_{Rec::alloc_val()}
#else
    : val_{Rec::alloc_val()}
#endif
  {
    if ( other.val_ != nullptr ) {
      memcpy( val_, other.val_, Rec::VAL_LEN );
    }
    memcpy( key_, other.key_, Rec::KEY_LEN );
  }

  Record & operator=( const Record & other )
  {
    if ( this != &other ) {
#if WITHLOC == 1
      loc_ = other.loc_;
#endif
      if ( other.val_ != nullptr ) {
        if ( val_ == nullptr ) { val_ = Rec::alloc_val(); }
        memcpy( val_, other.val_, Rec::VAL_LEN );
      }
      memcpy( key_, other.key_, Rec::KEY_LEN );
    }
    return *this;
  }

  Record( Record && other )
#if WITHLOC == 1
    : loc_{other.loc_}
#endif
  {
    uint8_t * v = val_;
    val_ = other.val_;
    other.val_ = v;
    memcpy( key_, other.key_, Rec::KEY_LEN );
  }

  Record & operator=( Record && other )
  {
    if ( this != &other ) {
#if WITHLOC == 1
      loc_ = other.loc_;
#endif
      uint8_t * v = val_;
      val_ = other.val_;
      other.val_ = v;
      memcpy( key_, other.key_, Rec::KEY_LEN );
    }
    return *this;
  }

  ~Record( void ) { Rec::dealloc_val( val_ ); }

  /* Accessors */
  const uint8_t * key( void ) const noexcept { return key_; }
  const uint8_t * val( void ) const noexcept { return val_; }
#if WITHLOC == 1
  uint64_t loc( void ) const noexcept { return loc_; }
#else
  uint64_t loc( void ) const noexcept { return 0; }
#endif

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
      uint64_t l = loc();
      io.write_all( reinterpret_cast<const char *>( &l ),
                    sizeof( uint64_t ) );
    }
  }
#if PACKED == 1
} __attribute__((packed));
#else
};
#endif

std::ostream & operator<<( std::ostream & o, const Record & r );

inline void iter_swap ( Record * a, Record * b ) noexcept
{
  std::swap( *a, *b );
}

#endif /* RECORD_T_HH */
