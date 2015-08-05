#ifndef RECORD_HH
#define RECORD_HH

#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>

#include "../config.h"

#ifdef HAVE_BOOST_POOL_POOL_ALLOC_HPP
#define USE_POOL 1
#endif

#if USE_POOL == 1
#include <boost/pool/pool_alloc.hpp>
#endif

#include "io_device.hh"

template <typename R1, typename R2>
int compare( const R1 & a, const R2 & b );

#define comp_op( op, t ) \
  bool operator op( const t & b ) const \
  { \
    return compare( b ) op 0 ? true : false; \
  }

class RecordPtr;

/**
 * Represent a record in the sort benchmark.
 *
 * The key is stored inline in the Record for efficient comparisson when
 * sorting Records, while the value is stored on the heap using a pool
 * allocator for efficient malloc/free.
 */
class Record
{
public:
  template <typename R1, typename R2>
  friend int compare( const R1 & a, const R2 & b );

  static constexpr size_t KEY_LEN = 10;
  static constexpr size_t VAL_LEN = 90;
  static constexpr size_t SIZE = KEY_LEN + VAL_LEN;

  static constexpr size_t LOC_LEN = sizeof( uint64_t );
  static constexpr size_t SIZE_WITH_LOC = SIZE + LOC_LEN;

  using key_type = char[KEY_LEN];
  using val_type = char[VAL_LEN];

  /* A Max or minimum record type. */
  enum limit_t { MAX, MIN };

  /* Should we include or not include disk location information? */
  enum loc_t { WITH_LOC, NO_LOC };

private:
  uint64_t loc_ = 0;
  char * val_ = nullptr;
  key_type key_;

  /* Use a pool allocator for speed */
#if USE_POOL == 1
  using Alloc = boost::fast_pool_allocator
    < val_type

    , boost::default_user_allocator_new_delete
    // , boost::default_user_allocator_malloc_free

    , boost::details::pool::default_mutex
    // , boost::details::pool::null_mutex

    // use a fixed growth size (42 ~ a page)
    , 42
    , 42
    >;
#endif

  static inline char * alloc_val( void )
  {
#if USE_POOL == 1
    return reinterpret_cast<char *>( Alloc::allocate() );
#else
    return new val_type;
#endif
  }

  static inline void dealloc_val( char * v )
  {
    if ( v != nullptr ) {
#if USE_POOL == 1
      Alloc::deallocate( reinterpret_cast<val_type *>( v ) );
#else
      delete v;
#endif
    }
  }

public:
  /* Default constructor */
  Record( void ) noexcept {}

  /* Construct a min or max record. */
  Record( limit_t lim ) noexcept
    : val_{alloc_val()}
  {
    if ( lim == MAX ) {
      memset( key_, 0xFF, KEY_LEN );
    } else {
      memset( key_, 0x00, KEY_LEN );
    }
  }

  /* Construct from c string read from disk */
  Record( const char * s, uint64_t loc = 0 )
    : loc_{loc}
    , val_{alloc_val()}
  {
    memcpy( key_, s, KEY_LEN );
    memcpy( val_, s+KEY_LEN, VAL_LEN );
  }

  /* Copy constructor */
  Record( const Record & other )
    : loc_{other.loc_}
    , val_{alloc_val()}
  {
    memcpy( key_, other.key_, KEY_LEN );
    if ( other.val_ != nullptr ) {
      memcpy( val_, other.val_, VAL_LEN );
    }
  }

  /* Copy assignment */
  Record & operator=( const Record & other )
  {
    if ( this != &other ) {
      loc_ = other.loc_;
      memcpy( key_, other.key_, KEY_LEN );
      if ( other.val_ != nullptr ) {
        if ( val_ == nullptr ) {
          val_ = alloc_val();
        }
        memcpy( val_, other.val_, VAL_LEN );
      }
    }
    return *this;
  }

  /* Move constructor */
  Record( Record && other )
    : loc_{other.loc_}
    , val_{other.val_}
  {
    memcpy( key_, other.key_, KEY_LEN );
    other.val_ = nullptr;
  }

  /* Move assignment */
  Record & operator=( Record && other )
  {
    if ( this != &other ) {
      dealloc_val( val_ );
      loc_ = other.loc_;
      val_ = other.val_;
      memcpy( key_, other.key_, KEY_LEN );
      other.val_ = nullptr;
    }
    return *this;
  }

  /* Destructor */
  ~Record( void ) { dealloc_val( val_ ); }

  /* Accessors */
  const char * key( void ) const noexcept { return key_; }
  const char * val( void ) const noexcept { return val_; }
  uint64_t loc( void ) const noexcept { return loc_; }

  /* methods for boost::sort */
  const char * data( void ) const noexcept { return key_; }
  unsigned char operator[]( size_t i ) const noexcept { return key_[i]; }
  size_t size( void ) const noexcept { return KEY_LEN; }

  /* comparison (with Record & RecordPtr) */
  comp_op( <, Record )
  comp_op( <, RecordPtr )
  comp_op( <=, Record )
  comp_op( <=, RecordPtr )
  comp_op( >, Record )
  comp_op( >, RecordPtr )

  int compare( const Record & b ) const { return ::compare( *this, b ); }
  int compare( const RecordPtr & b ) const { return ::compare( *this, b ); }

  /* To string */
  std::string str( loc_t locinfo = NO_LOC ) const
  {
    std::string buf( key_, KEY_LEN );
    buf.append( val_, VAL_LEN );

    if ( locinfo == WITH_LOC  ) {
      buf.append( reinterpret_cast<const char *>( &loc_  ),
                  sizeof( uint64_t  ) );
    }
    return buf;
  }

  /* Write to IO device */
  void write( IODevice & io, loc_t locinfo = NO_LOC ) const
  {
    io.write_all( key_, KEY_LEN );
    io.write_all( val_, VAL_LEN );
    if ( locinfo == WITH_LOC ) {
      io.write_all( reinterpret_cast<const char *>( &loc_ ),
                    sizeof( uint64_t ) );
    }
  }

} __attribute__((packed));

std::ostream & operator<<( std::ostream & o, const Record & r );

/* Wrapper around a string (and location) to enable comparison. Doens't manage
 * it's own storage. */
class RecordPtr
{
private:
  template <typename R1, typename R2>
  friend int compare( const R1 & a, const R2 & b );

  uint64_t loc_;
  const char * r_;

public:
  RecordPtr( const char * r, uint64_t loc ) : loc_{loc}, r_{r} {}

  RecordPtr( const RecordPtr & rptr ) = delete;
  RecordPtr & operator=( const RecordPtr & rptr ) = delete;
  RecordPtr( RecordPtr && rptr ) = delete;
  RecordPtr & operator=( RecordPtr && rptr ) = delete;

  const char * key( void ) const noexcept { return r_; }
  const char * val( void ) const noexcept { return r_ + Record::KEY_LEN; }
  uint64_t loc( void ) const noexcept { return loc_; }

  /* comparison (with Record & RecordPtr) */
  comp_op( <, Record )
  comp_op( <, RecordPtr )
  comp_op( <=, Record )
  comp_op( <=, RecordPtr )
  comp_op( >, Record )
  comp_op( >, RecordPtr )

  int compare( const Record & b ) const { return ::compare( *this, b ); }
  int compare( const RecordPtr & b ) const { return ::compare( *this, b ); }
};

/**
 * Record key + disk location
 */
class RecordLoc
{
public:
  template <typename R1, typename R2>
  friend int compare( const R1 & a, const R2 & b );

  using key_type = char[Record::KEY_LEN];

private:
  uint64_t loc_ = 0;
  key_type key_;

public:
  /* Default constructor */
  RecordLoc( void ) noexcept {}

  /* Construct from c string read from disk */
  RecordLoc( const char * s, uint64_t loc = 0 )
    : loc_{loc}
  {
    memcpy( key_, s, Record::KEY_LEN );
  }

  /* Copy constructor */
  RecordLoc( const RecordLoc & other )
    : loc_{other.loc_}
  {
    memcpy( key_, other.key_, Record::KEY_LEN );
  }

  /* Copy assignment */
  RecordLoc & operator=( const RecordLoc & other )
  {
    if ( this != &other ) {
      loc_ = other.loc_;
      memcpy( key_, other.key_, Record::KEY_LEN );
    }
    return *this;
  }

  /* Accessors */
  const char * key( void ) const noexcept { return key_; }
  uint64_t loc( void ) const noexcept { return loc_; }

  /* methods for boost::sort */
  const char * data( void ) const noexcept { return key_; }
  unsigned char operator[]( size_t i ) const noexcept { return key_[i]; }
  size_t size( void ) const noexcept { return Record::KEY_LEN; }

  /* comparison (with Record & RecordPtr) */
  comp_op( <, RecordLoc )
  comp_op( <=, RecordLoc )
  comp_op( >, RecordLoc )

  int compare( const RecordLoc & b ) const { return ::compare( *this, b ); }

} __attribute__((packed));

/* Comparison */
template <typename R1, typename R2>
int compare( const R1 & a, const R2 & b )
{
  // we compare on key first, and then on loc_
  int cmp = std::memcmp( a.key(), b.key(), Record::KEY_LEN );
  if ( cmp == 0 ) {
    if ( a.loc_ < b.loc_ ) {
      cmp = -1;
    }
    if ( a.loc_ > b.loc_ ) {
      cmp = 1;
    }
  }
  return cmp;
}

#endif /* RECORD_HH */
