/**
 * Basic index building test.
 *
 * - Uses BufferedIO file IO.
 * - Uses libsort record type.
 * - Use C++ std::set.
 */
#include <set>

#include "buffered_io.hh"
#include "exception.hh"
#include "file.hh"
#include "timestamp.hh"

#include "record_loc.hh"

using namespace std;

void run( char * fin )
{
  // get in/out files
  BufferedIO_O<File> fdi( {fin, O_RDONLY} );
  set<RecordLoc> recs;

  // read
  auto t0 = time_now();
  for ( uint64_t i = 0;; i++ ) {
    const uint8_t * r = (const uint8_t *) fdi.read_buf( Rec::SIZE ).first;
    if ( fdi.eof() ) {
      break;
    }
    recs.emplace( r, i * Rec::SIZE + Rec::KEY_LEN );
  }
  auto tt = time_diff<ms>( t0 );

  cout << "Total: " << tt << "ms" << endl;
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

