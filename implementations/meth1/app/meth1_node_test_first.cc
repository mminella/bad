/**
 * Print out the first record using method1::Node backend.
 */
#include <iostream>
#include <system_error>
#include <vector>

#include "timestamp.hh"
#include "record.hh"
#include "node.hh"

using namespace std;
using namespace meth1;

void run( vector<string> files )
{
  for ( auto & f : files ) {
    cout << f << endl;
  }
  auto t0 = time_now();

  // start node
  Node node{files, "0", true};
  node.Initialize();
  auto t1 = time_now();

  // size
  auto siz = node.Size();
  auto t2 = time_now();

  // read + write
  auto recs = node.Read( 0, 1 );
  if ( recs.size() == 1 ) {
    cout << "first: " << recs[0] << endl;
  } else {
    cerr << "error, got " << recs.size() << " records back" << endl;
  }
  auto t3 = time_now();

  // stats
  auto tinit = time_diff<ms>( t1, t0 );
  auto tsize = time_diff<ms>( t2, t1 );
  auto tread = time_diff<ms>( t3, t2 );
  auto ttota = time_diff<ms>( t3, t0 );

  cout << "-----------------------" << endl;
  cout << "Records " << siz << endl;
  cout << "-----------------------" << endl;
  cout << "Start took " << tinit << "ms" << endl;
  cout << "Size  took " << tsize << "ms" << endl;
  cout << "Read  took " << tread << "ms (read + sort)" << endl;
  cout << "Total took " << ttota << "ms" << endl;
}

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc < 2 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) + " [file..]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    run( {argv+1, argv+argc} );
  } catch ( const exception & e ) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

