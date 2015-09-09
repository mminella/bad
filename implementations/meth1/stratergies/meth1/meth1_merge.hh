#ifndef METH1_MERGE_HH
#define METH1_MERGE_HH

#include <iostream>
#include <vector>
#include <utility>

#include "merge.hh"
#include "timestamp.hh"

template <class T>
tdiff_t
meth1_merge_move( T * s1, T * e1, T * s2, T * e2, T * rs, T * re )
{
  auto t0 = time_now();
  merge_move( s1, e1, s2, e2, rs, re );
  return time_diff<ms>( t0 );
}

template <class T>
tdiff_t
meth1_merge_move( std::vector<T> & in1, std::vector<T> & in2,
                  std::vector<T> & out )
{
  auto t0 = time_now();
  merge_move( in1, in2, out );
  return time_diff<ms>( t0 );
}

/* cap -- capacity of S2 & R. */
template <class T>
tdiff_t
meth1_merge_copy( T * s1, T * e1, T * s2, T * e2, T * rs, size_t cap )
{
  auto t0 = time_now();
  auto sEnd = merge_copy( s1, e1, s2, e2, rs, rs + cap );

  // copy the end of s2 to the start of s1 to make s1 and rs independent of
  // each other. This is useful as it reduces the amount of memory we need to
  // allocate.
  size_t n1 = sEnd.first - s1;
  size_t n2 = sEnd.second - s2;
  if ( n1 > 0 ) {
    std::copy( s2 + cap - n1, s2 + cap, s1 );
  }
  // if we didn't max out R, then copy remaining parts of S2 into R.
  if ( n1 + n2 < cap ) {
    size_t nR = cap - n1 - n2;
    std::copy( sEnd.second, sEnd.second + nR, rs + n1 + n2 );
  }
  return time_diff<ms>( t0 );
}

/* cap -- capacity of S2 & R. */
template <typename T>
tdiff_t
meth1_pmerge_copy( T * s1, T * e1, T * s2, T * e2, T * rs, size_t cap )
{
  auto t0 = time_now();
  auto sEnd = pmerge_copy( s1, e1, s2, e2, rs, rs + cap );  

  // copy the end of s2 to the start of s1 to make s1 and rs independent of
  // each other. This is useful as it reduces the amount of memory we need to
  // allocate.
  size_t n1 = sEnd.first - s1;
  size_t n2 = sEnd.second - s2;
  if ( n1 > 0 ) {
    std::copy( s2 + cap - n1, s2 + cap, s1 );
  }
  // if we didn't max out R, then copy remaining parts of S2 into R.
  if ( n1 + n2 < cap ) {
    size_t nR = cap - n1 - n2;
    std::copy( sEnd.second, sEnd.second + nR, rs + n1 + n2 );
  }
  return time_diff<ms>( t0 );
}

template <typename T>
tdiff_t
meth1_pmerge_move( T * s1, T * e1, T * s2, T * e2, T * rs, T * re )
{
  auto t0 = time_now();
  pmerge_move( s1, e1, s2, e2, rs, re );  
  return time_diff<ms>( t0 );
}

#endif /* METH1_MERGE_HH */
