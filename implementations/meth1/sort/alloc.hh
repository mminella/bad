#ifndef ALLOC_HH
#define ALLOC_HH

/**
 * Allocation critical for performance. Best results have been:
 * - boost::pool (null lock) -- 60ms
 * - tcmalloc                -- 135ms
 * - jemalloc                -- 180ms
 * - hoard                   -- 205ms
 * - malloc (glibc)          -- 245ms
 * - new                     -- 285ms
 * - boost::pool (mutex)     -- 540ms
 */
#include "record_common.hh"

#include "../config.h"
#ifdef HAVE_BOOST_POOL_POOL_ALLOC_HPP
#define USE_POOL 0
#include <boost/pool/pool_alloc.hpp>
#endif

#if USE_POOL == 1
using val_type = uint8_t[Rec::VAL_LEN];

using Alloc = boost::fast_pool_allocator
  < val_type
  , boost::default_user_allocator_new_delete
  // , boost::details::pool::default_mutex
  , boost::details::pool::null_mutex
  // use a fixed growth size (42 ~ a page)
  , 42 , 42 >;
#endif

inline uint8_t * alloc_val( void )
{
#if USE_POOL == 1
  return (uint8_t *) Alloc::allocate();
#else
  return new uint8_t[Rec::VAL_LEN];
#endif
}

inline void dealloc_val( uint8_t * v )
{
  if ( v != nullptr ) {
#if USE_POOL == 1
    Alloc::deallocate( (val_type *) v );
#else
    delete v;
#endif
  }
}

#endif /* ALLOC_HH */
