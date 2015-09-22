#ifndef METH4_UTIL_HH
#define METH4_UTIL_HH

#include <iomanip>
#include <iostream>
#include <system_error>

#include <limits.h>
#include <unistd.h>

inline void reverse_bytes( void * bytes, size_t len )
{
  uint8_t * s = (uint8_t *) bytes;
  uint8_t * e = s + len - 1;
  while ( s < e ) {
    uint8_t tmp = *s;
    *s++ = *e;
    *e-- = tmp;
  }
}

inline size_t num_of_disks( void )
{
  FILE * fin = popen("ls /dev/xvd[b-z] 2>/dev/null | wc -l", "r");
  if ( not fin ) {
    throw std::runtime_error( "Couldn't count number of disks" );
  }
  char buf[256];
  char * n = fgets( buf, 256, fin );
  if ( n == nullptr ) {
    throw std::runtime_error( "Couldn't count number of disks" );
  }
  return std::max( (size_t) atoll( buf ), (size_t) 1 );
}

#endif /* METH4_UTIL_HH */
