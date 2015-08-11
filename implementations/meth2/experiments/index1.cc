/**
 * Basic index building test.
 *
 * - Uses BufferedIO file IO.
 * - Uses libsort record type.
 * - Use C++ std::sort method + std::vector.
 */
#include <algorithm>
#include <vector>

#include "buffered_io.hh"
#include "exception.hh"
#include "file.hh"
#include "timestamp.hh"

#include "record.hh"

#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
#include <boost/sort/spreadsort/string_sort.hpp>
using namespace boost::sort::spreadsort;
#endif

using namespace std;

void run( char * fin )
{
  // get in/out files
  BufferedIO_O<File> fdi( {fin, O_RDONLY} );
  size_t nrecs = fdi.io().size() / Record::SIZE;
  vector<RecordLoc> recs;
  recs.reserve( nrecs );
  auto t1 = time_now();

  // read
  for ( uint64_t i = 0;; i++ ) {
    const char * r = fdi.read_buf( Record::SIZE ).first;
    if ( fdi.eof() ) {
      break;
    }
    recs.emplace_back( r, i * Record::SIZE + Record::KEY_LEN );
  }
  auto t2 = time_now();

  // sort
#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
  string_sort( recs.begin(), recs.end() );
#else
  sort( recs.begin(), recs.end() );
#endif
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
