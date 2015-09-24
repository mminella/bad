#include <cstdio>
#include <iostream>
#include <system_error>

#include "config_file.hh"

using namespace std;

vector<Address> ConfigFile::parse( string file )
{
  FILE *fin = fopen( file.c_str(), "r" );
  if ( fin == nullptr ) {
    throw runtime_error( "Can't open cluster config file" );
  }

  vector<Address> cluster;
  char * line = nullptr;
  size_t llen = 0;
  while ( true ) {
    ssize_t n = getline( &line, &llen, fin );
    if ( n <= 0 ) {
      break;
    } else if ( n > 0 ) {
      // strip line endings
      if ( n > 2 and line[n-2] == '\r' and line[n-1] == '\n' ) {
        n -= 2;
      } else if ( n > 1 and line[n-1] == '\n' ) {
        n -= 1;
      }
      string host( line, n );

      size_t i = host.find_last_of( ':' );
      if ( i == string::npos ) {
        free( line );
        fclose( fin );
        throw runtime_error( "Badly formatted cluster config file" );
      }

      string hname = host.substr( 0, i );
      cluster.emplace_back( host, IPV4 );
    }
  }
  free( line );
  fclose( fin );

  return cluster;
}
