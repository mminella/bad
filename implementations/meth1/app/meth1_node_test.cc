/**
 * Do a local machine sort using the method1::Node backend.
 */
#include <chrono>
#include <iostream>
#include <system_error>

#include "exception.hh"

#include "record.hh"

#include "node.hh"

using namespace std;
using namespace meth1;

int run( char * fin, char * fout )
{
  BufferedIO_O<File> out( {fout, O_WRONLY | O_CREAT | O_TRUNC,
                                 S_IRUSR | S_IWUSR} );
  auto t1 = chrono::high_resolution_clock::now();

  // start node
  Node node{fin, "0", 3};
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
    r.write( out );
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
  cout << "Writ calls (buf) " << out.write_count() << endl;
  cout << "Writ calls (dir) " << out.io().write_count() << endl;

  return EXIT_SUCCESS;
}

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
    check_usage( argc, argv );
    run( argv[1], argv[2] );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

