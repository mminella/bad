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
  if ( argc != 3 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) + " [file] [out file]" );
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

  File out( argv[2], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR );

  Node node{argv[1], "0"};
  node.Initialize();

  size_t siz = node.Size();
  cout << "Size: " << siz << endl;

  auto recs = node.Read( 0, siz );
  cout << "Recs: " << recs.size() << endl;

  for ( auto & r : recs ) {
    out.write( r.str( Record::NO_LOC ) );
  }

  return EXIT_SUCCESS;
}
