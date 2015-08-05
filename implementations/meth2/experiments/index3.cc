/**
 * Basic index building test.
 *
 * - Uses BufferedIO file IO.
 * - Uses libsort record type.
 * - Use C++ std::sort method + std::vector.
 */
#include <algorithm>
#include <chrono>
#include <set>

#include "buffered_io.hh"
#include "exception.hh"
#include "file.hh"
#include "timestamp.hh"

#include "record.hh"
#include "btree_set.h"

#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
#include <boost/sort/spreadsort/string_sort.hpp>
using namespace boost::sort::spreadsort;
#endif

using namespace std;

int run( char * fin )
{
  // get in/out files
  BufferedIO_O<File> fdi( {fin, O_RDONLY} );
  btree::btree_set<RecordLoc, less<RecordLoc>, allocator<RecordLoc>, 1024> recs;

  // read
  auto t0 = time_now();
  for ( uint64_t i = 0;; i++ ) {
    const char * r = fdi.read_buf( Record::SIZE ).first;
    if ( fdi.eof() ) {
      break;
    }
    recs.insert( {r, i * Record::SIZE + Record::KEY_LEN} );
  }
  auto tt = time_diff<ms>( t0 );

  cout << "Total: " << tt << "ms" << endl;

  return EXIT_SUCCESS;
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

