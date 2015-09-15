#ifndef ALLOC_HH
#define ALLOC_HH

/**
 * Allocation critical for performance. Best results have been:
 * - MemoryPool              -- 60ms
 * - boost::pool (null lock) -- 60ms
 * - tcmalloc                -- 135ms
 * - jemalloc                -- 180ms
 * - hoard                   -- 205ms
 * - malloc (glibc)          -- 245ms
 * - new                     -- 285ms
 * - boost::pool (mutex)     -- 540ms
 */
#include "record_common.hh"

namespace Rec {

  inline uint8_t * alloc_val( void )
  {
    return new uint8_t[Rec::VAL_LEN];
  }

  inline void dealloc_val( uint8_t * v )
  {
    if ( v != nullptr ) {
      delete []v;
    }
  }
}

#endif /* ALLOC_HH */
