/**
 * Test program. Takes gensort file as input and reads whole thing into memory,
 * sorting using C++ algorithm sort implementation.
 */
#include <fcntl.h>

#include <algorithm>
#include <chrono>
#include <vector>

#include "buffered_io.hh"
#include "exception.hh"
#include "file.hh"
#include "util.hh"

#include "record.hh"

using namespace std;

int run( int argc, char * argv[] );

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc != 3 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) + " [file] [out]" );
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

  BufferedIO_O<File> fdi( {argv[1], O_RDONLY} );
  BufferedIO_O<File> fdo( {argv[2], O_WRONLY | O_CREAT | O_TRUNC,
                                    S_IRUSR | S_IWUSR} );

  vector<Record> recs;
  // PERF: avoid copying partial vectors
  recs.reserve( 10485700 );

  auto t1 = chrono::high_resolution_clock::now();

  // read
  for ( uint64_t i = 0;; i++ ) {
    // PERF: Avoid copy from buffer to string.
    const char * r = fdi.read_buf( Record::SIZE ).first;
    if ( fdi.eof() ) {
      break;
    }
    // PERF: Avoid copy into array, construct in-place.
    recs.emplace_back( r, i, true );
  }
  auto t2 = chrono::high_resolution_clock::now();

  // sort
  sort( recs.begin(), recs.end() );
  auto t3 = chrono::high_resolution_clock::now();

  // write
  for ( auto & r : recs ) {
    fdo.write_all( r.str( Record::NO_LOC ) );
  }
  fdo.flush( true );
  fdo.io().fsync();
  auto t4 = chrono::high_resolution_clock::now();

  // stats
  auto t21 = chrono::duration_cast<chrono::milliseconds>( t2 - t1 ).count();
  auto t32 = chrono::duration_cast<chrono::milliseconds>( t3 - t2 ).count();
  auto t43 = chrono::duration_cast<chrono::milliseconds>( t4 - t3 ).count();
  auto t41 = chrono::duration_cast<chrono::milliseconds>( t4 - t1 ).count();

  cout << "Read  took " << t21 << "ms" << endl;
  cout << "Sort  took " << t32 << "ms" << endl;
  cout << "Write took " << t43 << "ms" << endl;
  cout << "Total took " << t41 << "ms" << endl;
  cout << "Read calls (buf) " << fdi.read_count() << endl;
  cout << "Writ calls (buf) " << fdo.write_count() << endl;
  cout << "Read calls (dir) " << fdi.io().read_count() << endl;
  cout << "Writ calls (dir) " << fdo.io().write_count() << endl;

  return EXIT_SUCCESS;
}
