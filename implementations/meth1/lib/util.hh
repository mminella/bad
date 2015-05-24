#ifndef UTIL_HH
#define UTIL_HH

#include <cstring>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

/* Convert a string to hexadecimal form */
std::string str_to_hex( const std::string * in );
std::string str_to_hex( const uint8_t * const in, size_t size );

/* Collapse a vector of strings to a single spaced string */
const std::string join( const std::vector<std::string> & command );

/* Retrieve path to users default shell */
std::string shell_path( void );

/* prepend a string to the user's shell PS1 */
void prepend_shell_ps1( const char * prefix );

/* Clear user environ. Should be done when running as root. */
char ** clear_environ( void );

/* Drop root privileges */
void drop_privileges( void );

/* Various checks that running with right conditions */
void sanity_check_env( const int argc );
void check_suid_root( const std::string & prog );
void check_ip_forwarding( const std::string & prog );

/* Zero out an arbitrary structure */
template <typename T> void zero( T & x ) { memset( &x, 0, sizeof( x ) ); }

#endif /* UTIL_HH */
