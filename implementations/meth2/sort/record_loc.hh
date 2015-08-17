#ifndef RECORD_LOC_HH
#define RECORD_LOC_HH

#include <cstdint>
#include <cstring>

#include "record_common.hh"

/**
 * Record key + disk location
 */
class RecordLoc
{
public:
  uint64_t loc_;
  uint8_t key_[Rec::KEY_LEN];

  /* Default constructor */
  RecordLoc( void ) noexcept
    : loc_{0}
  {}

  /* Construct from c string read from disk */
  RecordLoc( const uint8_t * s, uint64_t loc = 0 )
    : loc_{loc}
  {
    memcpy( key_, s, 10 );
  }

  /* Copy constructor */
  RecordLoc( const RecordLoc & other )
    : loc_{other.loc_}
  {
    memcpy( key_, other.key_, 10 );
  }

  /* Copy assignment */
  RecordLoc & operator=( const RecordLoc & other )
  {
    if ( this != &other ) {
      loc_ = other.loc_;
      memcpy( key_, other.key_, 10 );
    }
    return *this;
  }

  /* methods for boost::sort */
  const char * data( void ) const noexcept { return (char *) key_; }
  unsigned char operator[]( size_t i ) const noexcept { return key_[i]; }
  size_t size( void ) const noexcept { return 10; }

  inline bool operator<( const RecordLoc & b ) const
  {
    return compare( b ) < 0 ? true : false;
  }

  inline int compare( const RecordLoc & b ) const
  {
    // we compare on key first, and then on loc_
    for ( size_t i = 0; i < Rec::KEY_LEN; i++ ) {
      if ( key_[i] != b.key_[i] ) {
        return key_[i] - b.key_[i];
      }
    }
    if ( loc_ < b.loc_ ) {
      return -1;
    }
    if ( loc_ > b.loc_ ) {
      return 1;
    }
    return 0;
  }

} __attribute__((packed));

#endif /* RECORD_LOC_HH */
