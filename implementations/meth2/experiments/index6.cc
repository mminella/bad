/**
 * Basic index building test.
 *
 * - Uses Overlapped file IO.
 * - Uses libsort record type.
 * - Uses google::btree.
 */
#include "exception.hh"
#include "file.hh"
#include "linux_compat.hh"
#include "overlapped_rec_io.hh"
#include "timestamp.hh"

#include "record.hh"
#include "btree_set.h"

using namespace std;

void run( char * fin )
{
  // open file
  File file( fin, O_RDONLY | O_DIRECT );
  OverlappedRecordIO<Rec::SIZE> rio( file );

  // stats
  size_t nrecs = file.size() / Rec::SIZE;
  cout << "size, " << nrecs << endl;
  cout << "sizeof, " << sizeof(RecordLoc) << endl;
  cout << endl;

  rio.rewind();

  // index
  btree::btree_set<RecordLoc, less<RecordLoc>, allocator<RecordLoc>, 227> bt;
  auto t1 = time_now();
  for ( uint64_t i = 0;; i++ ) {
    const uint8_t * r = (const uint8_t *) rio.next_record();
    if ( r == nullptr ) {
      break;
    }
    bt.insert( {r, i} );
  }
  auto t2 = time_now();

  // timings
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

