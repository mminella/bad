#include "alloc.hh"
#include "record_common.hh"

#if USE_MPOOL == 1
#include <iostream>
#include "MemoryPool.h"

namespace Rec {
  MemoryPool<uint8_t[Rec::VAL_LEN]> rec_pool;
}

#endif
