#include <iostream>
#include <cstring>
#include <stdio.h>

#include "record.hh"

using namespace std;

/* Construct an empty record. Guaranteed to be the maximum record. */
Record::Record( limit_t lim )
  : diskloc_ { 0 }
  , key_r_ { key_ }
  , val_r_ { val_ }
  , copied_ { true }
{
  if ( lim == MAX ) {
    memset( key_, 0xFF, KEY_LEN );
    memset( val_, 0xFF, VAL_LEN );
  } else {
    memset( key_, 0x00, KEY_LEN );
    memset( val_, 0x00, VAL_LEN );
  }
}

/* Copy external storage into our own storage */
void Record::copy( void )
{
  memcpy( key_, key_r_, KEY_LEN );
  memcpy( val_, val_r_, VAL_LEN );
  key_r_ = key_;
  val_r_ = val_;
  copied_ = true;
}

/* Construct from c string read from disk */
Record::Record( const char * s, size_type loc, bool copy )
  : diskloc_ { loc }
  , key_r_ { s }
  , val_r_ { s + KEY_LEN }
  , copied_ { copy }
{
  if ( copied_ ) { Record::copy(); }
}

/* Copy constructor */
Record::Record( const Record & other, bool deep_copy )
  : diskloc_ { other.diskloc_ }
  , key_r_ { other.key_r_ }
  , val_r_ { other.val_r_ }
  , copied_ { other.copied_ or deep_copy }
{
  if ( copied_ ) { copy(); }
}

/* Assignment */
Record & Record::operator=( const Record & other )
{
  if ( this != &other ) {
    diskloc_ = other.diskloc_;
    copied_ = other.copied_;
    key_r_ = other.key_r_;
    val_r_ = other.val_r_;

    if ( copied_ ) { copy(); }
  }
  return *this;
}

/* To string */
string Record::str( loc_t locinfo ) const
{
  string buf { key_r_, KEY_LEN };
  buf.append( val_, VAL_LEN );
  if ( locinfo == WITH_LOC ) {
    buf.append( reinterpret_cast<const char *>( &diskloc_ ),
                sizeof( size_type ) );
  }
  return buf;
}

Record Record::ParseRecord( const char * s, Record::size_type diskloc,
                            bool copy )
{
  return Record{ s, diskloc, copy };
}

Record Record::ParseRecordWithLoc( const char * s, bool copy )
{
  Record r { s, 0, copy };
  r.diskloc_ = *reinterpret_cast<const size_type *>( s + SIZE );
  return r;
}
