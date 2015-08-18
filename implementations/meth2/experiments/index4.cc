/**
 * Basic index building test.
 *
 * - Uses Overlapping IO.
 * - Uses libsort record type.
 * - Uses threadpool for parallelism.
 * - Use own chunking strategy.
 */
#include <algorithm>
#include <future>
#include <chrono>
#include <vector>

#include "buffered_io.hh"
#include "exception.hh"
#include "file.hh"
#include "overlapped_rec_io.hh"
#include "timestamp.hh"
#include "threadpool.hh"

#include "record.hh"

using namespace std;

using rec_i = vector<Record>::iterator;
using rec_ip = pair<rec_i, rec_i>;
using f_rec_ip = future<rec_ip>;
using s_rec_ip = shared_ptr<f_rec_ip>;

rec_ip mysort( uint64_t i, rec_i begin, rec_i end )
{
  (void) i;
  // cout << "start sort, " << i << ", " << timestamp<ms>() << endl;
  rec_sort( begin, end );
  // cout << "end sort, " << i << ", " << timestamp<ms>() << endl;
  rec_sort( begin, end );
  return make_pair( begin, end );
}

rec_ip mymerge( uint64_t i, s_rec_ip left, s_rec_ip right )
{
  (void) i;
  // cout << "queue merge, " << i << ", " << timestamp<ms>() << endl;
  auto l = left->get();
  auto r = right->get();
  // cout << "start merge, " << i << ", " << timestamp<ms>() << endl;
  inplace_merge( l.first, l.second, r.second );
  // cout << "end merge, " << i << ", " << timestamp<ms>() << endl;
  return make_pair( l.first, r.second );
}

void run( char * fin )
{
  // get in/out files
  File fdi( fin, O_RDONLY );
  OverlappedRecordIO<Rec::SIZE> rio( fdi );
  ThreadPool tp;

  // start reading
  rio.rewind();

  size_t nrecs = fdi.size() / Rec::SIZE;
  size_t split = 8;
  size_t chunk = nrecs / split;
  vector<Record> recs;
  recs.reserve( nrecs );
  vector<f_rec_ip> merges( split / 2 );

  auto t1 = time_now();
  uint64_t split_i = 0;
  bool run = true;
  for ( uint64_t i = 0; run; i++ ) {
    const char * r = rio.next_record();
    if ( fdi.eof() || r == nullptr ) {
      run = false;
    } else {
      recs.emplace_back( r, i * Rec::SIZE + Rec::KEY_LEN );
    }

    if ( i != 0 and ( i % chunk == 0 or !run )
        and recs.begin() + split_i < recs.end() ) {

      // start the sort
      auto fut = tp.enqueue( &mysort, split_i / chunk,
        recs.begin() + split_i,
        min( recs.begin() + split_i + chunk, recs.end() ) );
      split_i += chunk;

      // queue up the needed merges
      for ( auto & m : merges ) {
        if ( m.valid() ) {
          fut = tp.enqueue( &mymerge, split_i / chunk,
            make_shared<f_rec_ip>( move( m ) ),
            make_shared<f_rec_ip>( move( fut ) ) );
        } else {
          m = move( fut );
          break;
        }
      }
    }
  }
  auto t2 = time_now();

  // final merge
  auto tb = time_now();
  for ( const auto & m : merges ) {
    if ( m.valid() ) { m.wait(); }
  }
  auto t3 = time_now();

  // stats
  auto t21 = time_diff<ms>( t2, t1 );
  auto t32 = time_diff<ms>( t3, t2 );
  auto t31 = time_diff<ms>( t3, t1 );

  cout << "Read  took " << t21 << "ms" << endl;
  cout << "Sort  took " << t32 << "ms" << endl;
  cout << "Merge took " << time_diff<ms>( t3, tb ) << "ms" << endl;
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

