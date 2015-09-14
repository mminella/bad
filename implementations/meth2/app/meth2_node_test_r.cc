/**
 * Do a local machine sort using the method2::Node backend.
 */
#include <iostream>
#include <system_error>

#include "timestamp.hh"
#include "record.hh"
#include "node.hh"

using namespace std;
using namespace meth2;

static constexpr size_t MAX_MEM = 2097152; // 200MB

void run( char * fin, double block )
{
  auto t0 = time_now();

  // start node
  Node node{fin, "0", MAX_MEM, true};
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
  if ( argc != 2 and argc != 3 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) +
                         " [file] ([block %])" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    double block = argc == 3 ? stod( argv[2] ) : 1;
    run( argv[1], block );
  } catch ( const exception & e ) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

