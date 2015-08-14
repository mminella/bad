#ifndef RECORD_HH
#define RECORD_HH

#include "record_common.hh"
#include "record_ptr.hh"
#include "record_t.hh"

static inline int
compare( const unsigned char * k1, uint64_t loc1,
         const unsigned char * k2, uint64_t loc2 ) noexcept
{
  // we compare on key first, and then on loc
  for ( size_t i = 0; i < Rec::KEY_LEN; i++ ) {
    if ( k1[i] != k2[i] ) {
      return k1[i] - k2[i];
    }
  }
  if ( loc1 < loc2 ) {
    return -1;
  }
  if ( loc1 > loc2 ) {
    return 1;
  }
  return 0;
}


inline int Record::compare( const Record & b ) const
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}

inline int Record::compare( const RecordPtr & b ) const
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}

inline int RecordPtr::compare( const Record & b ) const
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}

inline int RecordPtr::compare( const RecordPtr & b ) const
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}

#endif /* RECORD_HH */
