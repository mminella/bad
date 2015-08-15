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
#define USE_BOOST 0
#define USE_MPOOL 1
#define USE_NEW   0

#include "record_common.hh"

#include "../config.h"
#if USE_POOL == 1 && defined(HAVE_BOOST_POOL_POOL_ALLOC_HPP)

#include <boost/pool/pool_alloc.hpp>
using val_type = uint8_t[Rec::VAL_LEN];
using Alloc = boost::fast_pool_allocator
  < val_type
  , boost::default_user_allocator_new_delete
  // , boost::details::pool::default_mutex
  , boost::details::pool::null_mutex
  // use a fixed growth size (42 ~ a page)
  , 42 , 42 >;

inline uint8_t * alloc_val( void )
{
  return (uint8_t *) Alloc::allocate();
}

inline void dealloc_val( uint8_t * v )
{
  if ( v != nullptr ) { Alloc::deallocate( (val_type *) v ); }
}

#elif USE_MPOOL == 1

#include <iostream>
#include "MemoryPool.h"
using val_type = uint8_t[Rec::VAL_LEN];
extern MemoryPool<val_type> rec_pool;

inline uint8_t * alloc_val( void )
{
  return (uint8_t *) rec_pool.allocate();
}

inline void dealloc_val( uint8_t * v )
{
  rec_pool.deallocate( (val_type *) v );
}

#else /* USE_NEW */
inline uint8_t * alloc_val( void )
{
  return new uint8_t[Rec::VAL_LEN];
}

inline void dealloc_val( uint8_t * v )
{
  if ( v != nullptr ) { delete v; }
}
#endif

#endif /* ALLOC_HH */
