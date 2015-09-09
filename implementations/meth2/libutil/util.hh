#ifndef UTIL_HH
#define UTIL_HH

#include <cstring>
#include <string>
#include <vector>

#include "raw_vector.hh"
#include "timestamp.hh"

/* Convert a string to hexadecimal form */
std::string str_to_hex( const std::string * in );
std::string str_to_hex( const char * const in, size_t size );
std::string str_to_hex( const uint8_t * const in, size_t size );

/* Convert a string to a bool */
bool to_bool( std::string str );

/* Collapse a vector of strings to a single spaced string */
const std::string join( const std::vector<std::string> & command );

/* Zero out an arbitrary structure */
template <typename T> inline void zero( T & x ) { memset( &x, 0, sizeof( x ) ); }

/* Physical memory present on the machine */
size_t memory_exists( void );

/* Physical memory present and free on the machine */
size_t memory_free( void );

/* A merge that move's elements rather than copy and doesn't have a stupid API
 * like STL */
template <class T>
tdiff_t move_merge ( std::vector<T> & in1, std::vector<T> & in2,
                     std::vector<T> & out )
{
  return move_merge( in1.data(), in1.data() + in1.size(),
                     in2.data(), in2.data() + in2.size(),
                     out.data(), out.data() + out.capacity() );
}

/* A merge that move's elements rather than copy and doesn't have a stupid API
 * like STL */
template <class T>
tdiff_t move_merge( T * s1, T * e1, T * s2, T * e2, T * rs, T * re )
{
  auto t0 = time_now();
  while ( s1 != e1 and s2 != e2 and rs != re ) {
    *rs++ = std::move( (*s2<*s1) ? *s2++ : *s1++ );
  }

  if ( rs != re ) {
    if ( s1 != e1 ) {
      std::move( s1, s1 + std::min( re - rs, e1 - s1 ), rs );
    } else if ( s2 != e2 ) {
      std::move( s2, s2 + std::min( re - rs, e2 - s2 ), rs );
    }
  }

  return time_diff<ms>( t0 );
}

/* Merge that does so with copy (not move) but with a better API than STL */
template <class T>
tdiff_t copy_merge( std::vector<T> & in1, std::vector<T> & in2,
                     std::vector<T> & out, bool independent = false )
{
  return copy_merge( in1.data(), in1.data() + in1.size(),
                     in2.data(), in2.data() + in2.size(),
                     out.data(), out.data() + out.capacity(),
                     independent );
}

/* Merge that does so with copy (not move) but with a better API than STL */
template <class T>
tdiff_t copy_merge( T * s1, T * e1, T * s2, T * e2, T * rs, T * re,
                    bool independent = false )
{
  auto t0 = time_now();
  T *s1s = s1;

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

  // Should we copy the end of s2 to the start of s1 to make s1 and rs
  // independent of each other? This can be useful in some merge strategies to
  // avoid uneseccary allocation / deallocation.
  auto i = s1 - s1s;
  if ( independent and i > 0 ) {
    std::copy( s2, s2 + i, s1s );
  }

  return time_diff<ms>( t0 );
}

/* A merge that move's elements rather than copy and doesn't have a stupid API
 * like STL */
template <class T>
size_t move_merge_n( RawVector<T> * in, size_t n, T * rs, T * re )
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

#endif /* UTIL_HH */

