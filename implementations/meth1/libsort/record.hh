#ifndef RECORD_HH
#define RECORD_HH

#include <algorithm>

#include "tune_knobs.hh"

#include "record_common.hh"
#include "record_loc.hh"
#include "record_ptr.hh"
#include "record_string.hh"
#include "record_t.hh"
#include "record_t_shallow.hh"

#include "config.h"

#ifdef HAVE_TBB_PARALLEL_SORT_H
#include "tbb/parallel_sort.h"
#endif

#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
#include <boost/sort/spreadsort/string_sort.hpp>
#endif

template <typename R>
inline void rec_sort( R first, R last )
{
#ifdef HAVE_TBB_PARALLEL_SORT_H
  if ( Knobs::PARALLEL_SORT ) {
    tbb::parallel_sort( first, last );
    return;
  }
#endif

#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
  boost::sort::spreadsort::string_sort( first, last );
#else
  sort( first, last, std::less<typename std::iterator_traits<R>::value_type>() );
#endif
}

inline int
own_memcmp( const uint8_t * k1, const uint8_t * k2 ) noexcept
{
  for ( size_t i = 0; i < Rec::KEY_LEN; i++ ) {
    if ( k1[i] != k2[i] ) {
      return k1[i] - k2[i];
    }
  }
  return 0;
}

inline int
compare( const uint8_t * k1, uint64_t loc1,
         const uint8_t * k2, uint64_t loc2 ) noexcept
{
  // we compare on key first, and then on loc
  int cmp;

  if ( Knobs::USE_OWN_MEMCMP ) {
    cmp = own_memcmp( k1, k2 );
  } else {
    cmp = memcmp( k1, k2, Rec::KEY_LEN );
  }

  if ( cmp != 0 ) {
    return cmp;
  } else {
#if WITHLOC == 1
    if ( loc1 < loc2 ) {
      return -1;
    }
    if ( loc1 > loc2 ) {
      return 1;
    }
#else
    (void) loc1; (void) loc2;
#endif
    return 0;
  }
}


/* RecordS */
inline int RecordS::compare( const uint8_t * k, uint64_t l ) const noexcept
{
  return ::compare( key(), loc(), k, l );
}

inline int RecordS::compare( const char * k, uint64_t l ) const noexcept
{
  return ::compare( key(), loc(), (const uint8_t *) k, l );
}

inline int RecordS::compare( const Record & b ) const noexcept
{
  return ::compare( key(), loc(), b.key(), b.loc() );
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

inline int Record::compare( const char * k, uint64_t l ) const noexcept
{
  return ::compare( key(), loc(), (const uint8_t *) k, l );
}

inline int Record::compare( const Record & b ) const noexcept
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}

inline int Record::compare( const RecordS & b ) const noexcept
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}

inline int Record::compare( const RecordPtr & b ) const noexcept
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}


/* RecordPtr */
inline int RecordPtr::compare( const uint8_t * k, uint64_t l ) const noexcept
{
  return ::compare( key(), loc(), k, l );
}

inline int RecordPtr::compare( const char * k, uint64_t l ) const noexcept
{
  return ::compare( key(), loc(), (const uint8_t *) k, l );
}

inline int RecordPtr::compare( const Record & b ) const noexcept
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}

inline int RecordPtr::compare( const RecordS & b ) const noexcept
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}

inline int RecordPtr::compare( const RecordPtr & b ) const noexcept
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}


/* RecordLoc */
inline int RecordLoc::compare( const uint8_t * k, uint64_t l ) const noexcept
{
  return ::compare( key(), loc(), k, l );
}

inline int RecordLoc::compare( const RecordLoc & b ) const noexcept
{
  return ::compare( key(), loc(), b.key(), b.loc() );
}


/* RecordString */
inline int RecordString::compare( const uint8_t * k ) const noexcept
{
  return ::compare( key(), 0, k, 0 );
}

inline int RecordString::compare( const RecordString &b ) const noexcept
{
  return ::compare( key(), 0, b.key(), 0 );
}

#endif /* RECORD_HH */
