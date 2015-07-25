#ifndef UTIL_HH
#define UTIL_HH

#include <cstring>
#include <string>
#include <vector>

#include "time.hh"

/* Convert a string to hexadecimal form */
std::string str_to_hex( const std::string * in );
std::string str_to_hex( const uint8_t * const in, size_t size );

/* Collapse a vector of strings to a single spaced string */
const std::string join( const std::vector<std::string> & command );

/* Zero out an arbitrary structure */
template <typename T> void zero( T & x ) { memset( &x, 0, sizeof( x ) ); }

/* A merge that move's elements rather than copy and doesn't have a stupid API
 * like STL */
template <class T>
tdiff_t move_merge ( std::vector<T> & in1, std::vector<T> & in2,
                     std::vector<T> & out )
{
  auto t0 = clk::now();

  T *s1 = in1.data(), *s2 = in2.data(), *rs = out.data();
  T *e1 = s1 + in1.size(), *e2 = s2 + in2.size(), *re = rs + out.capacity();

  while ( s1 != e1 and s2 != e2 and rs != re ) {
    *rs++ = std::move( (*s2<*s1)? *s2++ : *s1++ );
  }

  if ( rs != re ) {
    if ( s1 != e1 ) {
      std::move( s1, s1 + std::min( re - rs, e1 - s1 ), rs );
    } else if ( s2 != e2 ) {
      std::move( s2, s2 + std::min( re - rs, e2 - s2 ), rs );
    }
  }

  return time_diff( t0 );
}

#endif /* UTIL_HH */
