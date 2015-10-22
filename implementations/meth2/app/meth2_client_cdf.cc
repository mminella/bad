#include <fcntl.h>

#include <algorithm>
#include <iostream>
#include <vector>

#include "exception.hh"
#include "util.hh"

#include "record.hh"

#include "cluster.hh"

using namespace std;
using namespace meth2;

int run( int argc, char * argv[] );

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc < 3 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) +
                         " [points] [nodes...]" );
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

  char ** addresses = argv + 2;

  uint64_t readahead = 16;
  auto addrs = vector<Address>( addresses, argv + argc );
  Cluster c{addrs, readahead};
  
  auto size = c.Size();
  cout << "Size: " << size << endl;
  uint64_t points = stoul( argv[1] );

  auto start = time_now();
  for (uint64_t i = 0; i < points; i++) {
      c.Read(size * i / points, 1);
  }
  auto stop = time_now();
  auto diff = time_diff<ms>(start);
  cout << "Time: " << diff << "mS" << endl;

  return EXIT_SUCCESS;
}

