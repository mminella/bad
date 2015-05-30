#include <fcntl.h>

#include <algorithm>
#include <vector>

#include "exception.hh"
#include "util.hh"

#include "record.hh"

#include "node.hh"

using namespace std;
using namespace meth1;

int run( int argc, char * argv[] );

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc != 2 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) + " [file]" );
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
  sanity_check_env( argc );
  check_usage( argc, argv );

  Node node { argv[1] };
  node.Initialize();
  node.Run(); 

  return EXIT_SUCCESS;
}

