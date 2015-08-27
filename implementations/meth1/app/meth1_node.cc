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

int run( vector<string> files, string port, string odirect )
{
  Node node{files, port, to_bool( odirect )};
  node.Initialize();
  node.Run();

  return EXIT_SUCCESS;
}

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc < 4 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) +
                         " [port] [odirect] [file...]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    run( {argv+3, argv+argc}, argv[1], argv[2] );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

