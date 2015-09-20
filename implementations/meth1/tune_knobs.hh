#ifndef TUNE_KNOBS_HH
#define TUNE_KNOBS_HH

/**
 * We collect all the tuneable parameters for method 1 here.
 */

#include <cstdint>
#include <string>

namespace Knobs {
  /* Network send & receive kernel buffer sizes */
  static constexpr std::size_t NET_SND_BUF = std::size_t( 1024 ) * 1024 * 2;
  static constexpr std::size_t NET_RCV_BUF = std::size_t( 1024 ) * 1024 * 2;

  /* Overlapped IO buffer sizes .*/
  static constexpr uint64_t IO_BLOCK = 4096 * 256 * 10; // 10MB
  static constexpr uint64_t DISK_BLOCKS = 400;          // 4000MB

  /* Buffered (not overlapped) IO size */
  static constexpr uint64_t IO_BUFFER_DEFAULT = 1024 * 1024;

  /* Network write buffer size (measured in records, not bytes) */
  static constexpr uint64_t IO_BUFFER_NETW = 1024 * 1024 * 5; // 500MB

  /* Size (in records) of client writer buffer */
  static constexpr uint64_t CLIENT_WRITE_BUFFER = 1024 * 100; // 10MB

  /* Memory to leave unused for OS and other misc purposes. */
  static constexpr uint64_t MEM_RESERVE = 0;

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

  /* Use hand-rolled memcmp? */
  static constexpr bool USE_OWN_MEMCMP = true;

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
