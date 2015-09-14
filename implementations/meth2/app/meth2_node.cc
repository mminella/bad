/**
 * Run the backend node for method 2.
 */
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>

#include "exception.hh"
#include "record.hh"
#include "node.hh"

using namespace std;
using namespace meth2;

bool to_bool( std::string str )
{
  std::transform( str.begin(), str.end(), str.begin(), ::tolower );
  std::istringstream is( str );
  bool b;
  is >> std::boolalpha >> b;
  return b;
}

int run( const char * port, vector<string> files)
{
  Node node{files, port};
  node.Initialize();
  node.Run();

  return EXIT_SUCCESS;
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
    run( argv[1], { argv+2, argv+argc } );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

