#include <fcntl.h>

#include <algorithm>
#include <vector>

#include "exception.hh"
#include "file.hh"
#include "util.hh"

#include "node.hh"

#include "record.hh"

using namespace std;

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

  Imp1 imp1 { argv[1] };
  imp1.Initialize();
  
  auto recs = imp1.Read(1, 5);
  for ( auto & r : recs ) {
    cout << "Record: " << r.offset() << endl;
  }
  
  return EXIT_SUCCESS;
}

