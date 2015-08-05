/**
 * Basic index building test. Same as index1 but using overlapped io.
 */
#include <algorithm>
#include <future>
#include <chrono>
#include <vector>

#include "buffered_io.hh"
#include "exception.hh"
#include "file.hh"
#include "timestamp.hh"

#include "record.hh"
#include "olio.hh"
#include "threadpool.hh"

#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
#include <boost/sort/spreadsort/string_sort.hpp>
using namespace boost::sort::spreadsort;
#endif

using namespace std;

void mysort( vector<RecordLoc>::iterator begin, vector<RecordLoc>::iterator end )
{
  return sort( begin, end );
}

int run( char * fin )
{
  // get in/out files
  File fdi( fin, O_RDONLY );
  OverlappedRecordIO<Record::SIZE> olio( fdi );
  ThreadPool tp;

  size_t nrecs = fdi.size() / Record::SIZE;
  size_t split = nrecs / 2;
  vector<RecordLoc> recs;
  recs.reserve( nrecs );
  auto t1 = time_now();
  future<void> f;

  // read
  olio.rewind();
  for ( uint64_t i = 0;; i++ ) {
    const char * r = olio.next_record();
    if ( fdi.eof() || r == nullptr ) {
      break;
    }
    recs.emplace_back( r, i * Record::SIZE + Record::KEY_LEN );
    if ( i == split ) {
      f = tp.enqueue( &mysort, recs.begin(), recs.begin() + split );
    }
  }
  auto t2 = time_now();

  // sort
  sort( recs.begin() + split, recs.end() );
  f.get();
  auto tb = time_now();
  inplace_merge( recs.begin(), recs.begin() + split, recs.end() );
  auto t3 = time_now();

  // stats
  auto t21 = time_diff<ms>( t2, t1 );
  auto t32 = time_diff<ms>( t3, t2 );
  auto t31 = time_diff<ms>( t3, t1 );

  cout << "Read  took " << t21 << "ms" << endl;
  cout << "Sort  took " << t32 << "ms" << endl;
  cout << "Merge took " << time_diff<ms>( t3, tb ) << "ms" << endl;
  cout << "Total took " << t31 << "ms" << endl;

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

