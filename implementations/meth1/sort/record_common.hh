#ifndef RECORD_COMMON_HH
#define RECORD_COMMON_HH

#include <cstdint>
#include <cstring>

/* Use parallel sort? */
#define PARALLEL_SORT 1

/* Use packed data structure? */
#define PACKED 1

/* Include disk offset information? Technically the algorithm could break
 * without this, as we need a total order even when we have duplicate keys.
 * However, it's only a problem if we have a chunk boundary fall on a duplicate
 * key. Given we'll generally have < 100 chunk boundaries, and duplicate keys
 * aren't common, it's generally fine to not include location information. */
#define WITHLOC 1

#define comp_op( op, t ) \
  bool operator op( const t & b ) const noexcept \
  { \
    return compare( b ) op 0 ? true : false; \
  }

class RecordPtr;
class Record;
class RecordS;

namespace Rec {
  constexpr size_t KEY_LEN = 10;
  constexpr size_t VAL_LEN = 90;
  constexpr size_t SIZE = KEY_LEN + VAL_LEN;

  constexpr size_t LOC_LEN = sizeof( uint64_t );
  constexpr size_t SIZE_WITH_LOC = SIZE + LOC_LEN;

  /* A Max or minimum record type. */
  enum limit_t { MAX, MIN };

  /* Should we include or not include disk location information? */
  enum loc_t { WITH_LOC, NO_LOC };
}

#endif /* RECORD_COMMON_HH */
