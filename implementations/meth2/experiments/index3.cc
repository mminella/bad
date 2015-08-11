/**
 * Basic index building test.
 *
 * - Uses BufferedIO file IO.
 * - Uses libsort record type.
 * - Uses google::btree.
 */
#include <vector>

#include "buffered_io.hh"
#include "exception.hh"
#include "file.hh"
#include "timestamp.hh"

#include "record.hh"
#include "btree_set.h"

using namespace std;

void run( char * fin )
{
  // get in/out files
  BufferedIO_O<File> fdi( {fin, O_RDONLY} );
  size_t nrecs = fdi.io().size() / 100;
  vector<RecordLoc> recs;
  recs.reserve( nrecs );
  auto t1 = time_now();

  // read
  for ( uint64_t i = 0;; i++ ) {
    const char * r = fdi.read_buf( 100 ).first;
    if ( fdi.eof() ) {
      break;
    }
    recs.emplace_back( r, i * 100 + 10 );
  }
  auto t2 = time_now();

  btree::btree_set<RecordLoc, less<RecordLoc>, allocator<RecordLoc>, 1024> bt;
  for ( const auto & r : recs ) {
    bt.insert( r );
  }
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

