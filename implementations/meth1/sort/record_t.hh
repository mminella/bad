#ifndef RECORD_T_HH
#define RECORD_T_HH

#include <cstdint>
#include <cstring>
#include <iostream>

#include "io_device.hh"

#include "alloc.hh"
#include "record_common.hh"

/**
 * Represent a record in the sort benchmark.
 */
class Record
{
private:
  unsigned char * val_ = nullptr;
  uint64_t loc_ = 0;
  unsigned char key_[Rec::KEY_LEN];

public:
  Record( void ) noexcept {}

  /* Construct a min or max record. */
  Record( Rec::limit_t lim ) noexcept
    : val_{alloc_val()}
  {
    if ( lim == Rec::MAX ) {
      memset( key_, 0xFF, Rec::KEY_LEN );
    } else {
      memset( key_, 0x00, Rec::KEY_LEN );
    }
  }

  /* Construct from c string read from disk */
  Record( const char * s, uint64_t loc = 0 )
    : val_{alloc_val()}
    , loc_{loc}
  {
    memcpy( key_, s, Rec::KEY_LEN );
    memcpy( val_, s + Rec::KEY_LEN, Rec::VAL_LEN );
  }

  Record( const Record & other )
    : val_{alloc_val()}
    , loc_{other.loc_}
  {
    memcpy( key_, other.key_, Rec::KEY_LEN );
    if ( other.val_ != nullptr ) {
      memcpy( val_, other.val_, Rec::VAL_LEN );
    }
  }

  Record & operator=( const Record & other )
  {
    if ( this != &other ) {
      loc_ = other.loc_;
      memcpy( key_, other.key_, Rec::KEY_LEN );
      if ( other.val_ != nullptr ) {
        if ( val_ == nullptr ) { val_ = alloc_val(); }
        memcpy( val_, other.val_, Rec::VAL_LEN );
      }
    }
    return *this;
  }

  Record( Record && other )
    : val_{other.val_}
    , loc_{other.loc_}
  {
    memcpy( key_, other.key_, Rec::KEY_LEN );
    other.val_ = nullptr;
  }

  Record & operator=( Record && other )
  {
    if ( this != &other ) {
      dealloc_val( val_ );
      loc_ = other.loc_;
      val_ = other.val_;
      memcpy( key_, other.key_, Rec::KEY_LEN );
      other.val_ = nullptr;
    }
    return *this;
  }

  ~Record( void ) { dealloc_val( val_ ); }

  /* Accessors */
  const unsigned char * key( void ) const noexcept { return key_; }
  const unsigned char * val( void ) const noexcept { return val_; }
  uint64_t loc( void ) const noexcept { return loc_; }

  /* methods for boost::sort */
  const char * data( void ) const noexcept { return (char *) key_; }
  unsigned char operator[]( size_t i ) const noexcept { return key_[i]; }
  size_t size( void ) const noexcept { return Rec::KEY_LEN; }

  /* comparison */
  comp_op( <, Record )
  comp_op( <, RecordPtr )
  comp_op( <=, Record )
  comp_op( <=, RecordPtr )
  comp_op( >, Record )
  comp_op( >, RecordPtr )

  int compare( const Record & b ) const;
  int compare( const RecordPtr & b ) const;

  /* To string */
  std::string str( Rec::loc_t locinfo = Rec::NO_LOC ) const
  {
    std::string buf( (char *) key_, Rec::KEY_LEN );
    buf.append( (char *) val_, Rec::VAL_LEN );

    if ( locinfo == Rec::WITH_LOC  ) {
      buf.append( reinterpret_cast<const char *>( &loc_  ),
                  sizeof( uint64_t  ) );
    }
    return buf;
  }

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

} __attribute__((packed));

std::ostream & operator<<( std::ostream & o, const Record & r );

#endif /* RECORD_T_HH */
