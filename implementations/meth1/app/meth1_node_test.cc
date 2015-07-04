#include <fcntl.h>

#include <algorithm>
#include <chrono>
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
    throw runtime_error( "Usage: " + string( argv[0] ) +
                         " [file] [out file]" );
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
  // startup
  check_usage( argc, argv );

  BufferedIO_O<File> out( {argv[2], O_WRONLY | O_CREAT | O_TRUNC,
                                    S_IRUSR | S_IWUSR} );
  auto t1 = chrono::high_resolution_clock::now();

  // start node
  Node node{argv[1], "0", 3};
  node.Initialize();
  auto t2 = chrono::high_resolution_clock::now();

  // size
  size_t siz = node.Size();
  auto t3 = chrono::high_resolution_clock::now();

  // read records + sort
  auto recs = node.Read( 0, siz );
  auto t4 = chrono::high_resolution_clock::now();

  // write out records
  for ( auto & r : recs ) {
    out.write_all( r.str( Record::NO_LOC ) );
  }
  out.flush( true );
  out.io().fsync();
  auto t5 = chrono::high_resolution_clock::now();

  // stats
  auto t21 = chrono::duration_cast<chrono::milliseconds>( t2 - t1 ).count();
  auto t32 = chrono::duration_cast<chrono::milliseconds>( t3 - t2 ).count();
  auto t43 = chrono::duration_cast<chrono::milliseconds>( t4 - t3 ).count();
  auto t54 = chrono::duration_cast<chrono::milliseconds>( t5 - t4 ).count();
  auto t51 = chrono::duration_cast<chrono::milliseconds>( t5 - t1 ).count();

  cout << "-----------------------" << endl;
  cout << "Size       " << siz << endl;
  cout << "Records    " << recs.size() << endl;
  cout << "-----------------------" << endl;
  cout << "Start took " << t21 << "ms" << endl;
  cout << "Size  took " << t32 << "ms" << endl;
  cout << "Read  took " << t43 << "ms (buffered + sort)" << endl;
  cout << "Write took " << t54 << "ms" << endl;
  cout << "Total took " << t51 << "ms" << endl;

  return EXIT_SUCCESS;
}
