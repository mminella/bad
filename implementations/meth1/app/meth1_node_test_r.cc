/**
 * Do a local machine sort using the method1::Node backend.
 */
#include <iostream>
#include <system_error>
#include <vector>

#include "timestamp.hh"
#include "record.hh"
#include "node.hh"

using namespace std;
using namespace meth1;

void run( vector<string> files, double block )
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
  auto block_size = block * siz;
  for ( size_t i = 0; i < siz; i += block_size ) {
    auto recs = node.Read( i, block_size );
    cout << "last: " << recs[recs.size()-1] << endl;
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
  if ( argc < 3 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) +
                         " [block %] [file...]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    run( {argv+2, argv+argc}, stod( argv[1] ) );
  } catch ( const exception & e ) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

