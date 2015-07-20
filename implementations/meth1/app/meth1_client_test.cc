#include <fcntl.h>

#include <algorithm>
#include <vector>

#include "exception.hh"
#include "util.hh"

#include "record.hh"
#include "implementation.hh"

#include "cluster.hh"

using namespace std;
using namespace meth1;

int run( int argc, char * argv[] );

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc <= 4 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) +
                         " [size] [read ahead] [out file] [nodes...]" );
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

  uint64_t records = stoul( argv[1] );
  size_t read_ahead = stoul( argv[2] );
  string file_out{argv[3]};
  char ** addresses = argv + 4;

  File out( file_out, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR );

  auto addrs = vector<Address>( addresses, argv + argc );
  Cluster client{addrs, read_ahead};
  client.Initialize();

  auto recs = client.Read( 0, records );
  for ( const auto & r : recs ) {
    out.write( r.str( Record::NO_LOC ) );
  }

  return EXIT_SUCCESS;
}
