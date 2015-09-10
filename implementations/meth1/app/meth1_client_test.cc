#include <fcntl.h>

#include <algorithm>
#include <iostream>
#include <vector>

#include "exception.hh"
#include "util.hh"

#include "record.hh"

#include "cluster.hh"

using namespace std;
using namespace meth1;

int run( int argc, char * argv[] );

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc < 4 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) +
                         " [read ahead] [out file] [nodes...]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    run( argc, argv );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int run( int argc, char * argv[] )
{
  check_usage( argc, argv );

  size_t readahead = stoul( argv[1] );
  string ofile{argv[2]};
  char ** addresses = argv + 3;

  File out( ofile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR );

  auto addrs = vector<Address>( addresses, argv + argc );
  Cluster c{addrs, readahead};

  auto size = c.Size();
  cout << "Size: " << size << endl;

  Record r = c.ReadFirst();
  cout << "Record: " << r << endl;

  c.ReadAll();

  c.WriteAll( move( out ) );

  return EXIT_SUCCESS;
}

