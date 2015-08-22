#ifndef MERGE_WRAPPER_HH
#define MERGE_WRAPPER_HH

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

template <class T>
tdiff_t
meth1_merge_copy( T * s1, T * e1, T * s2, T * e2, T * rs, T * re )
{
  auto t0 = time_now();
  auto sEnd = merge_copy( s1, e1, s2, e2, rs, re );

  // copy the end of s2 to the start of s1 to make s1 and rs independent of
  // each other. This is useful as it reduces the amount of memory we need to
  // allocate.
  auto i = sEnd.first - s1;
  if ( i > 0 ) {
    std::copy( sEnd.second, sEnd.second + i, s1 );
  }
  return time_diff<ms>( t0 );
}

template <typename T>
tdiff_t
meth1_pmerge_copy( T * s1, T * e1, T * s2, T * e2, T * rs, T * re )
{
  auto t0 = time_now();
  auto sEnd = pmerge_copy( s1, e1, s2, e2, rs, re );  

  // copy the end of s2 to the start of s1 to make s1 and rs independent of
  // each other. This is useful as it reduces the amount of memory we need to
  // allocate.
  auto i = sEnd.first - s1;
  if ( i > 0 ) {
    std::copy( sEnd.second, sEnd.second + i, s1 );
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

#endif /* MERGE_WRAPPER_HH */
