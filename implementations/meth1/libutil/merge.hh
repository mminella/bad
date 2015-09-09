#ifndef MERGE_HH
#define MERGE_HH

/**
 * Collection of useful merge functions with better API's than STL. Main API
 * difference is we support specifying the end of the buffer to be merged into, so
 * that partial prefix merges can be performed.
 */

#include <algorithm>
#include <vector>
#include <utility>

#include "config.h"

#ifdef HAVE_TBB_PARALLEL_INVOKE_H
#include "tbb/parallel_invoke.h"
#endif

#include "raw_vector.hh"

// Granularity to switch to sequential merge at?
static constexpr size_t SPLIT_MIN = 50000;

/* INTERNAL: merge (copy) -- sets final s1 merged-to point in `s1f` reference
 * variable. We only set `s1f` if we merge some of s1 but not all of it.
 * This allows us to use a single shared variable in a parallel merge, as that
 * condition will only occur in one thread */
template <typename T>
void
__merge_copy( T * s1, T * e1, T * s2, T * e2, T * rs, T * re, T * & s1f )
{
  T * s1s = s1;

  while ( s1 != e1 and s2 != e2 and rs != re ) {
    *rs++ = (*s2<*s1) ? *s2++ : *s1++;
  }

  if ( rs != re ) {
    if ( s1 != e1 ) {
      auto n = std::min( re - rs, e1 - s1 );
      std::copy( s1, s1 + n, rs );
      s1 += n;
    } else if ( s2 != e2 ) {
      auto n = std::min( re - rs, e2 - s2 );
      std::copy( s2, s2 + std::min( re - rs, e2 - s2 ), rs );
      s2 += n;
    }
  }
  
  if ( s1 - s1s > 0 and s1 != e1 ) {
    s1f = s1;
  }
}

/* Merge using copy. Returns the iterator at which we finished merging from
 * each buffer. This may be less than their ends as the output buffer may have
 * become full first. */
template <typename T>
std::pair<T*, T*>
merge_copy( T * s1, T * e1, T * s2, T * e2, T * rs, T * re )
{
  if ( rs >= re ) {
    return std::make_pair( s1, s2 );
  }

  T * s1f = nullptr;
  __merge_copy( s1, e1, s2, e2, rs, re, s1f );
  if ( s1f == nullptr ) {
    s1f = e1;
  }

  auto nE = std::min( re - rs, e1 - s1 + e2 - s2 );
  auto n1 = s1f - s1;
  auto n2 = nE - n1;
  return std::make_pair( s1 + n1, s2 + n2 ); 
}

/* Merge using move. */
template <class T>
std::pair<T*, T*>
merge_copy ( std::vector<T> & in1, std::vector<T> & in2, std::vector<T> & out )
{
  return merge_copy( in1.data(), in1.data() + in1.size(),
                     in2.data(), in2.data() + in2.size(),
                     out.data(), out.data() + out.capacity() );
}

/* Merge using move. */
template <class T>
void
merge_move( T * s1, T * e1, T * s2, T * e2, T * rs, T * re )
{
  while ( s1 < e1 and s2 < e2 and rs < re ) {
    *rs++ = std::move( (*s2<*s1) ? *s2++ : *s1++ );
  }

  if ( rs < re ) {
    if ( s1 < e1 ) {
      std::move( s1, s1 + std::min( re - rs, e1 - s1 ), rs );
    } else if ( s2 < e2 ) {
      std::move( s2, s2 + std::min( re - rs, e2 - s2 ), rs );
    }
  }
}

/* Merge using move. */
template <class T>
void
merge_move ( std::vector<T> & in1, std::vector<T> & in2, std::vector<T> & out )
{
  merge_move( in1.data(), in1.data() + in1.size(),
              in2.data(), in2.data() + in2.size(),
              out.data(), out.data() + out.capacity() );
}

/* Merge (n-way) using move. */
template <class T>
size_t
merge_move_n( RawVector<T> * in, size_t n, T * rs, T * re )
{
  size_t nout = 0;
  size_t * pos = new size_t[n];
  for ( size_t i = 0; i < n; i++ ) {
    pos[i] = 0;
  }
  
  while ( rs != re ) {
    T * min = nullptr;
    size_t which_i = 0;
    
    for ( size_t i = 0; i < n; i++ ) {
      if ( pos[i] < in[i].size() ) {
        if ( min == nullptr or *min > in[i][pos[i]] ) {
          min = &in[i][pos[i]];
          which_i = i;
        }
      }
    }

    if ( min != nullptr ) {
      *rs++ = std::move( *min );
      pos[which_i]++;
      nout++;
    } else {
      break;
    }
  }

  return nout;
}

/* INTERNAL: A parralel merge using copy. See `__merge_copy` for a description
 * of the interface. We use Intel TBB for managing concurrency, although only
 * very lightly (worker pool).  */
template <typename T>
void
__pmerge_copy( T * s1, T * e1, T * s2, T * e2, T * rs, T * re, T * & s1f )
{
  if ( re <= rs or ( s1 >= e1 and s2 >= e2 ) ) {
    return;
  } else if ( s1 >= e1 ) {
    auto n = std::min( re - rs, e2 - s2 );
    std::copy( s2, s2 + n, rs );
    return;
  } else if ( s2 >= e2 ) {
    auto n = std::min( re - rs, e1 - s1 );
    std::copy( s1, s1 + n, rs );
    if ( s1 + n < e1 ) {
      s1f = s1 + n;
    }
    return;
  }

  size_t len1 = e1 - s1, len2 = e2 - s2, lenR = re - rs;
  if ( len1 + len2 < SPLIT_MIN or lenR < SPLIT_MIN ) {
    __merge_copy( s1, e1, s2, e2, rs, re, s1f );
  } else {
    size_t mid = len2 < lenR ? len2 / 2 : lenR / 2;

    T *m2 = &s2[mid];
    T *m1 = std::lower_bound( s1, e1, *m2 );

    T *ss1 = s1, *ee1 = m1;
    T *ss2 = s2, *ee2 = s2 + mid;
    T *rrs = rs, *rre = std::min( re, rs + (ee2 - ss2) + (ee1 - ss1) );

#ifdef HAVE_TBB_PARALLEL_INVOKE_H
    tbb::parallel_invoke(
      [&] { __pmerge_copy( ss1, ee1, ss2, ee2, rrs, rre, s1f ); },
      [&] { __pmerge_copy( ee1, e1, ee2, e2, rre, re, s1f ); }
    );
#else
    __pmerge_copy( ss1, ee1, ss2, ee2, rrs, rre, s1f );
    __pmerge_copy( ee1, e1, ee2, e2, rre, re, s1f );
#endif
  }
}

/* A parallel merge using copy. Returns the iterator at which we finished
 * merging from each buffer. This may be less than their ends as the output
 * buffer may have become full first. */
template <typename T>
std::pair<T*,T*>
pmerge_copy( T * s1, T * e1, T * s2, T * e2, T * rs, T * re )
{
  if ( rs >= re ) {
    return std::make_pair( s1, s2 );
  }

  T * s1f = nullptr;
  __pmerge_copy( s1, e1, s2, e2, rs, re, s1f );
  if ( s1f == nullptr ) {
    s1f = e1;
  }

  auto nE = std::min( re - rs, e1 - s1 + e2 - s2 );
  auto n1 = s1f - s1;
  auto n2 = nE - n1;
  return std::make_pair( s1 + n1, s2 + n2 ); 
}

/* A parallel merge using move. */
template <typename T>
void
pmerge_move( T * s1, T * e1, T * s2, T * e2, T * rs, T * re )
{
  if ( re <= rs or ( s1 >= e1 and s2 >= e2 ) ) {
    return;
  } else if ( s1 >= e1 ) {
    auto n = std::min( re - rs, e2 - s2 );
    std::move( s2, s2 + n, rs );
    return;
  } else if ( s2 >= e2 ) {
    auto n = std::min( re - rs, e1 - s1 );
    std::move( s1, s1 + n, rs );
    return;
  }

  size_t len1 = e1 - s1, len2 = e2 - s2, lenR = re - rs;
  if ( len1 + len2 < SPLIT_MIN or lenR < SPLIT_MIN ) {
    merge_move( s1, e1, s2, e2, rs, re );
  } else {
    size_t mid = len2 < lenR ? len2 / 2 : lenR / 2;

    T *m2 = &s2[mid];
    T *m1 = std::lower_bound( s1, e1, *m2 );

    T *ss1 = s1, *ee1 = m1;
    T *ss2 = s2, *ee2 = s2 + mid;
    T *rrs = rs, *rre = std::min( re, rs + (ee2 - ss2) + (ee1 - ss1) );

#ifdef HAVE_TBB_PARALLEL_INVOKE_H
    tbb::parallel_invoke(
      [&] { pmerge_move( ss1, ee1, ss2, ee2, rrs, rre ); },
      [&] { pmerge_move( ee1, e1, ee2, e2, rre, re ); }
    );
#else
    pmerge_move( ss1, ee1, ss2, ee2, rrs, rre );
    pmerge_move( ee1, e1, ee2, e2, rre, re );
#endif
  }
}

#endif /* MERGE_HH */
