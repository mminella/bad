#include <fcntl.h>

#include <algorithm>
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
  if ( argc <= 1 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) + " [nodes...]" );
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

  auto addrs = vector<Address>( argv+1, argv+argc );
  Cluster client { addrs };
  client.Initialize();
  
  auto recs = client.Read(0, 5);
  cout << "Recs: " << recs.size() << endl;
  for ( auto & r : recs ) {
    cout << "Record: " << r.diskloc() << endl;
  }

  auto siz = client.Size();
  cout << "Size: " << siz << endl;
  
  return EXIT_SUCCESS;
}


