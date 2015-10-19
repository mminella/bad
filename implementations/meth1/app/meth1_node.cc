/**
 * Run the backend node for method 1.
 */
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include "exception.hh"
#include "util.hh"

#include "record.hh"
#include "node.hh"

using namespace std;
using namespace meth1;

void run( vector<string> files, string port )
{
  Node node{files, port};
  node.Initialize();
  node.Run();
}

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc < 3 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) +
                         " [port] [file...]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    run( {argv+2, argv+argc}, argv[1] );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

