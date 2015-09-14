/**
 * Basic index building test.
 *
 * - Uses Overlapping file IO.
 * - Uses libsort record type.
 * - Use C++ std::sort method + std::vector or boost::sort if available.
 * - Use multi-buffer sort + merge approach.
 */
#include <sys/stat.h>
#include <future>
#include <iostream>

#include "file.hh"
#include "linux_compat.hh"
#include "overlapped_rec_io.hh"
#include "raw_vector.hh"
#include "timestamp.hh" 
#include "threadpool.hh"
#include "util.hh"
#include "merge.hh"

#include "record.hh"

using namespace std;

using RR = RecordLoc;

tdiff_t mysort( RR * first, RR * last )
{
  auto t1 = time_now();
  rec_sort( first, last );
  return time_diff<ms>( t1 );
}

RR * scan( OverlappedRecordIO<Rec::SIZE> & rio, size_t size )
{
  auto t0 = time_now();
  tdiff_t tsort = 0;
  ThreadPool tp;

  rio.rewind();

  // some hardcoded split points for sorting + merge overlap.
  // - on my machine, 75% for the first sort is a good point.
  // - remaining 25% should also be sorted in chunks to make
  //   final (after IO) part of algorithm just cheaper merges.
  size_t split_at1 = size / 4 * 3;
  size_t split_at2 = (split_at1 + size) / 2;
  future<tdiff_t> fsort1, fsort2;
  RR * r1 = new RR[size];
  RR * r2 = new RR[size];

  for ( size_t i = 0;; i++ ) {
    // TODO: we could use a zero copy interface here, but unlikely to have an
    // significant impact.
    const uint8_t * r = (const uint8_t *) rio.next_record();
    if ( r == nullptr ) {
      break;
    }
    r1[i].copy( r, i );

    if ( i == split_at1 ) {
      fsort1 = tp.enqueue( &mysort, r1, r1 + split_at1 );
    } else if ( i == split_at2 ) {
      auto tt = time_now();
      fsort2 = tp.enqueue( &mysort, r1 + split_at1, r1 + split_at2 );
      tsort += time_diff<ms>( tt );
    }
  }

  auto t1 = time_now();
  rec_sort( r1 + split_at2, r1 + size );
  auto t2 = time_now();

  // inplace merge the final 25% two splits.
  tsort += fsort2.get();
  inplace_merge( r1 + split_at1, r1 + split_at2, r1 + size );
  tsort += fsort1.get();

  // external merge the 75%, 25% file chunks.
  merge_copy( r1, r1 + split_at1, r1 + split_at1, r1 + size, r2, r2 + size );

  // ALTERNATIVE: n-way merge the three chunks.
  // RawVector<RR> * rv = new RawVector<RR>[3];
  // rv[0] = { r1, split_at };
  // rv[1] = { r1 + split_at, split2 - split_at };
  // rv[2] = { r1 + split2, size - split2 };
  // move_merge_n( rv, 3, r2, r2 + size );

  auto t3 = time_now();

  // validate
  for ( size_t i = 1; i < size; i++ ) {
    if ( r2[i-1] > r2[i] ) {
      cerr << "Out-of-order Record: " << i << endl;
    }
  }

  // tsort += time_diff<ms>( t2, t1 ) + f.get();
  auto tmerg = time_diff<ms>( t3, t2 );
  auto tread = time_diff<ms>( t1, t0 );
  auto twall = time_diff<ms>( t3, t0 );

  cout << "sort, " << tsort << endl;
  cout << "merg, " << tmerg << endl;
  cout << "wall, " << twall << endl;
  cout << "time, " << tread + tsort + tmerg << endl;
  cout << "extr, " << twall - tread << endl;
  
  // return r2;
  return nullptr;
}

void run( char * fin )
{
  // open file
  File file( fin, O_RDONLY | O_DIRECT );
  OverlappedRecordIO<Rec::SIZE> rio( file );

  // stats
  size_t nrecs = file.size() / Rec::SIZE;
  cout << "size, " << nrecs << endl;
  cout << endl;

  // build index
  scan( rio, nrecs );
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

