/**
 * Basic index building test.
 *
 * - Uses BufferedIO file IO.
 * - Uses libsort record type.
 * - Uses insertion sort.
 */
#include <algorithm>
#include <vector>

#include "buffered_io.hh"
#include "exception.hh"
#include "file.hh"
#include "timestamp.hh"

#include "record.hh"

using namespace std;

template<typename T>
typename std::vector<T>::iterator 
insert_sorted( std::vector<T> & vec, T const& item )
{
  return vec.insert ( 
    std::upper_bound( vec.begin(), vec.end(), item ),
    item 
  );
}

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
    insert_sorted( recs, {r, i * Rec::SIZE + Rec::KEY_LEN} );
  }
  auto t2 = time_now();

  // stats
  auto t21 = time_diff<ms>( t2, t1 );

  cout << "Total took " << t21 << "ms" << endl;
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

