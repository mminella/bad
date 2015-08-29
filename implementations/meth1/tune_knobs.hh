#ifndef TUNE_KNOBS_HH
#define TUNE_KNOBS_HH

/**
 * We collect all the tuneable parameters for method 1 here.
 */

#include <cstdint>

namespace Knobs {
  /* Overlapped IO buffer sizes .*/
  static constexpr uint64_t IO_BLOCK = 4096 * 256 * 10; // 10MB
  static constexpr uint64_t IO_NBLOCKS = 200;           // 2GB

  /* Buffered (not overlapped) IO size */
  static constexpr uint64_t IO_BUFFER = 1024 * 1024;

  /* Maximum copy buffer a client should use. The copy buffer is used to copy
   * data of the wire once we less than the copy buffer size is remaining on
   * the wire. This gives us some chance that the next read request to that
   * backend will overlap with other backends. A biffer buffer is a better
   * chance, but also means increase copying. */
  static constexpr uint64_t CLIENT_MAX_BUFFER = 1024 * 1024 * 1000 * uint64_t( 5 );

  /* Memory to leave unused for OS and other misc purposes. */
  static constexpr uint64_t MEM_RESERVE = 1024 * 1024 * 1000 * uint64_t( 3 );

  /* How much smaller ( 1 / SORT_MERGE_RATIO ) should the sort buffer be than
   * the merge buffer? */
  static constexpr uint64_t SORT_MERGE_RATIO = 14;

  /* Lower bound on sort buffer size. */
  static constexpr uint64_t SORT_MERGE_LOWER = 3145728;

  /* We can use a move or copy strategy -- the copy is actaully a little better
   * as we play some tricks to ensure we reuse allocations as much as possible.
   * With copy we use `size + r1x` value memory, but with move, we use up to
   * `2.size + r1x`. */
  static constexpr bool USE_COPY = true;

  /* We can reuse our sort+merge buffers for a big win! Be careful though, as
   * the results returned by scan are invalidate when you next call scan. */
  static constexpr bool REUSE_MEM = true;

  /* Use a parallel merge implementation? */
  static constexpr bool PARALLEL_MERGE = true;

  /* Use parallel sort? */
  static constexpr bool PARALLEL_SORT = true;

  /* Record -- use packed data structure? */
  #define PACKED 1

  /* Record -- include disk offset information? Technically the algorithm could
   * break without this, as we need a total order even when we have duplicate
   * keys.  However, it's only a problem if we have a chunk boundary fall on a
   * duplicate key. Given we'll generally have < 100 chunk boundaries, and
   * duplicate keys aren't common, it's generally fine to not include location
   * information. */
  #define WITHLOC 0
}

#endif /* TUNE_KNOBS_HH */
