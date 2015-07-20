/**
 * Run the backend node for method 1.
 */
#include <system_error>

#include "exception.hh"
#include "record.hh"
#include "node.hh"

using namespace std;
using namespace meth1;

int run( const char * mem, const char * port, const char * file)
{

  Node node{file, port, stoul( mem )};
  node.Initialize();
  node.Run();

  return EXIT_SUCCESS;
}

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc != 4 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) +
                         " [max mem] [port] [file]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    run( argv[1], argv[2], argv[3] );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

