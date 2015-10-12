#include <cstdio>
#include <iostream>
#include <system_error>

#include "config_file.hh"

using namespace std;

pair<Address, vector<Address>> ConfigFile::parse( string file )
{
  FILE *fin = fopen( file.c_str(), "r" );
  if ( fin == nullptr ) {
    throw runtime_error( "Can't open cluster config file" );
  }

  bool first = true;
  Address client;
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
      if ( first ) {
        client = Address( string( line, n ), IPV4 );
        first = false;
      } else {
        cluster.emplace_back( string( line, n ), IPV4 );
      }
    }
  }
  free( line );
  fclose( fin );

  return make_pair( client, cluster );
}
