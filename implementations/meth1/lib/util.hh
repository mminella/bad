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

/* Collapse a vector of strings to a single spaced string */
const std::string join( const std::vector<std::string> & command );

/* Zero out an arbitrary structure */
template <typename T> inline void zero( T & x ) { memset( &x, 0, sizeof( x ) ); }

#endif /* UTIL_HH */
