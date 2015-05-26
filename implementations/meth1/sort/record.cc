#include <cstring>
#include <stdio.h>

#include "record.hh"

/* Construct an empty record. Guaranteed to be the maximum record. */
Record::Record( limit_t lim )
  : offset_ { 0 }
  , key_r_ { key_ }
  , val_r_ { val_ }
  , copied_ { true }
{
  if ( lim == MAX ) {
    std::memset( key_, 0xFF, KEY_LEN );
    std::memset( val_, 0xFF, VAL_LEN );
  } else {
    std::memset( key_, 0x00, KEY_LEN );
    std::memset( val_, 0x00, VAL_LEN );
  }
}

/* Construct from c string read from disk */
Record::Record( uint64_t offset, const char * s, bool copy )
  : offset_ { offset }
  , key_r_ { reinterpret_cast<const uint8_t *>( s ) }
  , val_r_ { reinterpret_cast<const uint8_t *>( s + KEY_LEN ) }
  , copied_ { copy }
{
  if ( copy ) {
    std::memcpy( key_, s , KEY_LEN );
    std::memcpy( val_, s + KEY_LEN , VAL_LEN );
    key_r_ = key_;
    val_r_ = val_;
  }
}

/* Copy given record into our own storage */
void Record::copy( const Record & other )
{
  std::memcpy( key_, other.key_r_, KEY_LEN );
  std::memcpy( val_, other.val_r_, VAL_LEN );
  key_r_ = key_;
  val_r_ = val_;
  copied_ = true;
}

/* Move constructor */
Record::Record( Record && other )
  : offset_ { other.offset_ }
  , key_r_ { other.key_r_ }
  , val_r_ { other.val_r_ }
  , copied_ { other.copied_ }
{
  if ( copied_ ) {
    copy( other );
  }
}

/* Copy constructor */
Record::Record( const Record & other, bool deep_copy )
  : offset_ { other.offset_ }
  , key_r_ { other.key_r_ }
  , val_r_ { other.val_r_ }
  , copied_ { other.copied_ || deep_copy }
{
  if ( copied_ ) {
    copy ( other );
  }
}

/* Assignment */
Record & Record::operator=( const Record & other )
{
  if ( this == &other ) {
    return *this;
  }

  offset_ = other.offset_;
  copied_ = other.copied_;

  if ( copied_ ) {
    copy( other );
  } else {
    key_r_ = other.key_r_;
    val_r_ = other.val_r_;
  }

  return *this;
}

/* Destructor */
Record::~Record( void )
{
}

/* Clone method to make a deep copy */
Record Record::clone( void )
{
  return { *this, true };
}

/* Serialization */
std::string Record::str( void ) const
{
  char buf[Record::SIZE];
  std::memcpy( buf, key_r_, KEY_LEN );
  std::memcpy( buf + KEY_LEN, val_, VAL_LEN );
  return std::string( buf, Record::SIZE );
}
