/**
 * Test performance of priority_queue's.
 *
 * - Uses C file IO.
 * - Uses own Record struct.
 * - Use C++ std::priority_queue + std::vector.
 */
#include <sys/stat.h>
#include <iostream>

#include "file.hh"
#include "overlapped_rec_io.hh"
#include "record.hh"
#include "timestamp.hh" 

#include "config.h"
#include "pq.hh"

#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
#include <boost/sort/spreadsort/string_sort.hpp>
using namespace boost::sort::spreadsort;
#endif

using namespace std;

using RR = RecordS;

vector<RR> scan( OverlappedRecordIO<Rec::SIZE> & rio, size_t size, const RR & after )
{
  auto t0 = time_now();
  size_t cmps = 0, pushes = 0, pops = 0;

  rio.rewind();

  mystl::priority_queue<RR> pq{size + 1};
  uint64_t i = 0;

  while ( true ) {
    const char * r = rio.next_record();
    if ( r == nullptr ) {
      break;
    }
    RecordPtr next( r, i++ );
    cmps++;
    if ( next > after ) {
      if ( pq.size() < size ) {
        pushes++;
        pq.emplace( r, i );
      } else if ( next < pq.top() ) {
        cmps++; pushes++; pops++;
        pq.emplace( r, i );
        pq.pop();
      } else {
        cmps++;
      }
    }
  }
  auto t1 = time_now();

  vector<RR> vrecs = move( pq.container() );
#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
  string_sort( vrecs.begin(), vrecs.end() );
#else
  sort( vrecs.begin(), vrecs.end() );
#endif

  cout << "pq, " << time_diff<ms>( t1, t0 ) << endl;
  cout << "* cmps   , " << cmps << endl;
  cout << "* push   , " << pushes << endl;
  cout << "* pops   , " << pops << endl;
  cout << endl;

  return vrecs;
}

void run( char * fin )
{
  // open file
  File file( fin, O_RDONLY | O_DIRECT );
  OverlappedRecordIO<Rec::SIZE> rio( file );

  // stats
  size_t nrecs = file.size() / Rec::SIZE;
  size_t split = 20;
  size_t chunk = nrecs / split;
  cout << "size, " << nrecs << endl;
  cout << "cunk, " << chunk << endl;
  cout << endl;

  // starting record
  RR after( Rec::MIN );

  // scan file
  auto t1 = time_now();
  for ( uint64_t i = 0; i < 1; i++ ) {
    auto recs = scan( rio, chunk, after );
    after = recs.back();
    cout << "last: " << recs[recs.size()-1] << endl;
  }
  cout << endl << "total: " << time_diff<ms>( t1 ) << endl;
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

