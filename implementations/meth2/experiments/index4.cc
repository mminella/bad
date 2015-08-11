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
#include "timestamp.hh"

#include "record.hh"
#include "olio.hh"
#include "threadpool.hh"

#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
#include <boost/sort/spreadsort/string_sort.hpp>
using namespace boost::sort::spreadsort;
#endif

using namespace std;

using rec_i = vector<Record>::iterator;
using rec_ip = pair<rec_i, rec_i>;
using f_rec_ip = future<rec_ip>;
using s_rec_ip = shared_ptr<f_rec_ip>;

rec_ip mysort( rec_i begin, rec_i end )
{
  sort( begin, end );
  return make_pair( begin, end );
}

rec_ip mymerge( s_rec_ip left, s_rec_ip right )
{
  auto l = left->get();
  auto r = right->get();
  inplace_merge( l.first, l.second, r.second );
  return make_pair( l.first, r.second );
}

void run( char * fin )
{
  // get in/out files
  File fdi( fin, O_RDONLY );
  OverlappedRecordIO<Record::SIZE> olio( fdi );
  ThreadPool tp;

  size_t nrecs = fdi.size() / Record::SIZE;
  vector<Record> recs;
  recs.reserve( nrecs );

  // size_t split = nrecs / 4;
  // vector<f_rec_ip> merge( 4 );

  // read
  auto t1 = time_now();
  olio.rewind();
  // uint64_t split_i = 0;
  bool run = true;
  for ( uint64_t i = 0; run; i++ ) {
    const char * r = olio.next_record();
    if ( fdi.eof() || r == nullptr ) {
      run = false;
    } else {
      recs.emplace_back( r, i * Record::SIZE + Record::KEY_LEN );
    }

  //   // if ( i != 0 and ( i % split == 0 or !run ) and recs.begin() + split_i < recs.end() ) {
  //   //   auto f1 = tp.enqueue( &mysort, recs.begin() + split_i,
  //   //                                  min( recs.begin() + split_i + split, recs.end() ) );
  //   //   split_i += split;
  //   //   if ( merge[0].valid() ) {
  //   //     auto f2 = tp.enqueue( &mymerge, make_shared<f_rec_ip>( move( merge[0] ) ),
  //   //                                     make_shared<f_rec_ip>( move( f1 ) ) );
  //   //     if ( merge[1].valid() ) {
  //   //       auto f3 = tp.enqueue( &mymerge, make_shared<f_rec_ip>( move( merge[1] ) ),
  //   //                                       make_shared<f_rec_ip>( move( f2 ) ) );
  //   //       if ( merge[2].valid() ) {
  //   //         auto f4 = tp.enqueue( &mymerge, make_shared<f_rec_ip>( move( merge[2] ) ),
  //   //                                         make_shared<f_rec_ip>( move( f3 ) ) );
  //   //         merge[3] = move( f4 );
  //   //       } else {
  //   //         merge[2] = move( f3 );
  //   //       }
  //   //     } else {
  //   //       merge[1] = move( f2 );
  //   //     }
  //   //   } else {
  //   //     merge[0] = move( f1 );
  //   //   }
    // }
  }
  auto t2 = time_now();

  // sort
  mysort( recs.begin(), recs.begin() + nrecs / 2 );
  mysort( recs.begin() + nrecs / 2, recs.end() );
  // mysort( recs.begin(), recs.end() );

  // // merge
  auto tb = time_now();
  vector<Record> recs2;
  recs2.reserve( nrecs );
  // merge( recs2.begin(), recs2.end(), recs.begin(), recs.begin() + nrecs / 2, recs.begin() + nrecs / 2, recs.end() );
  // inplace_merge( recs.begin(), recs.begin() + nrecs / 2, recs.end() );
  // // merge[2].wait();
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

