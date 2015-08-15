#ifndef RECORD_HH
#define RECORD_HH

#include "record_common.hh"
#include "record_ptr.hh"
#include "record_t.hh"
#include "record_t_shallow.hh"

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


/* RecordS */
inline int RecordS::compare( const uint8_t * k, uint64_t l ) const noexcept
{
  return ::compare( key(), loc(), k, l );
}

inline int RecordS::compare( const RecordS & b ) const noexcept
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}

inline int RecordS::compare( const RecordPtr & b ) const noexcept
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}


/* Record */
inline int Record::compare( const uint8_t * k, uint64_t l ) const noexcept
{
  return ::compare( key(), loc(), k, l );
}

inline int Record::compare( const Record & b ) const noexcept
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}

inline int Record::compare( const RecordPtr & b ) const noexcept
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}


/* RecordPtr */
inline int RecordPtr::compare( const Record & b ) const noexcept
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}

inline int RecordPtr::compare( const RecordPtr & b ) const noexcept
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}

#endif /* RECORD_HH */
