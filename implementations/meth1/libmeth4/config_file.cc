#include <cstdio>
#include <iostream>
#include <system_error>

#include "config_file.hh"

using namespace std;

ConfigFile::ConfigFile( string file, string hostname )
  : myID_{0}
  , cluster_{}
{
  if ( hostname.size() == 0 ) {
    throw runtime_error( "Hostname to config file can't be empty" );
  }

  FILE *fin = fopen( file.c_str(), "r" );
  if ( fin == nullptr ) {
    throw runtime_error( "Can't open cluster config file" );
  }

  bool myIDset = false;
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
      if ( hname == hostname ) {
        myID_ = cluster_.size();
        myIDset = true;
      }
      cluster_.emplace_back( host, IPV4 );
    }
  }
  free( line );
  fclose( fin );

  if ( not myIDset ) {
    throw runtime_error( "Didn't find own host in configuration file" );
  }
}

uint16_t ConfigFile::myID( void ) const noexcept
{
  return myID_;
}

vector<Address> ConfigFile::cluster( void ) const noexcept
{
  return cluster_;
}
