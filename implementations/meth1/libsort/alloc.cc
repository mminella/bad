#include "alloc.hh"
#include "record_common.hh"

#if USE_MPOOL == 1
#include <iostream>
#include "MemoryPool.h"

namespace Rec {
  Alloc rec_pool;
}

#endif
