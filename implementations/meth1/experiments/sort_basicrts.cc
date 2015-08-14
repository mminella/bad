/**
 * Adaptation of `sort_libc` to use libbasicrts.
 *
 * - Uses BufferedIO file IO.
 * - Uses libsort record type.
 * - Use C++ std::sort method + std::vector.
 */
#include <algorithm>
#include <chrono>
#include <vector>

#include "buffered_io.hh"
#include "exception.hh"
#include "file.hh"

#include "record.hh"

using namespace std;

int run( char * fin, char * fout )
{
  // get in/out files
  BufferedIO_O<File> fdi( {fin, O_RDONLY} );
  BufferedIO_O<File> fdo( {fout, O_WRONLY | O_CREAT | O_TRUNC,
                                 S_IRUSR | S_IWUSR} );
  size_t nrecs = fdi.io().size() / Rec::SIZE;
  vector<Record> recs;
  recs.reserve( nrecs );

  auto t1 = chrono::high_resolution_clock::now();

  // read
  for ( uint64_t i = 0;; i++ ) {
    const char * r = fdi.read_buf( Rec::SIZE ).first;
    if ( fdi.eof() ) {
      break;
    }
    recs.emplace_back( r, i );
  }
  auto t2 = chrono::high_resolution_clock::now();

  // sort
  sort( recs.begin(), recs.end() );
  auto t3 = chrono::high_resolution_clock::now();

  // write
  for ( auto & r : recs ) {
    r.write( fdo );
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

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc != 3 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) + " [file] [out]" );
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

