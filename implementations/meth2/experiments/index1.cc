/**
 * Basic index building test.
 *
 * - Uses BufferedIO file IO.
 * - Uses libsort record type.
 * - Use C++ std::sort method + std::vector or boost::sort if available.
 */
#include <algorithm>
#include <vector>

#include "buffered_io.hh"
#include "exception.hh"
#include "file.hh"
#include "timestamp.hh"

#include "record.hh"

using namespace std;

void run( char * fin )
{
  // get in/out files
  BufferedIO_O<File> fdi( {fin, O_RDONLY} );
  size_t nrecs = fdi.io().size() / Rec::SIZE;
  vector<RecordLoc> recs;
  recs.reserve( nrecs );
  auto t1 = time_now();

  // read
  for ( uint64_t i = 0;; i++ ) {
    const uint8_t * r = (const uint8_t *) fdi.read_buf( Rec::SIZE ).first;
    if ( fdi.eof() ) {
      break;
    }
    recs.emplace_back( r, i * Rec::SIZE + Rec::KEY_LEN );
  }
  auto t2 = time_now();

  // sort
  rec_sort( recs.begin(), recs.end() );
  auto t3 = time_now();

  // stats
  auto t21 = time_diff<ms>( t2, t1 );
  auto t32 = time_diff<ms>( t3, t2 );
  auto t31 = time_diff<ms>( t3, t1 );

  cout << "Read  took " << t21 << "ms" << endl;
  cout << "Sort  took " << t32 << "ms" << endl;
  cout << "Total took " << t31 << "ms" << endl;
}

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc != 2 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) + " [file]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    run( argv[1] );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

