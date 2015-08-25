/**
 * Test how quickly we can just read from disk, filtering some keys and storing
 * into memory.
 */
#include <sys/stat.h>
#include <iostream>

#include "file.hh"
#include "overlapped_rec_io.hh"
#include "record.hh"
#include "timestamp.hh" 
#include "util.hh"

#include "config.h"
#include "merge_wrapper.hh"

using namespace std;

using RR = Record;

void run( char * fin )
{
  // open file
  File file( fin, O_RDONLY | O_DIRECT );
  OverlappedRecordIO<Rec::SIZE> rio( file );
  size_t nrecs = file.size() / Rec::SIZE;

  // use first record as a filter
  auto * r = rio.next_record();
  RR after( r );

  rio.rewind();

  // filter file
  auto t0 = time_now();
  RR * recs = new RR[nrecs];
  size_t zrecs = 0;
  for ( size_t i = 0; i < nrecs; i++ ) {
    r = rio.next_record();
    if ( r == nullptr ) {
      break;
    }
    RecordPtr next( r );
    if ( after < next ) {
      recs[zrecs++].copy( next );
    }
  }
  cout << "time , " << time_diff<ms>( t0 ) << endl;
  cout << "zrecs, " << zrecs << endl;
  cout << endl;
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
    cerr << e.what() << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

