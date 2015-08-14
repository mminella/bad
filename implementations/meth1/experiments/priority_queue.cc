/**
 * Test performance of priority_queue's.
 *
 * - Uses C file IO.
 * - Uses own Record struct.
 * - Use C++ std::priority_queue + std::vector.
 */
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <queue>
#include <system_error>
#include <vector>

#include "config.h"

#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
#include <boost/heap/binomial_heap.hpp>
#include <boost/heap/d_ary_heap.hpp>
#include <boost/heap/priority_queue.hpp>
#include <boost/heap/fibonacci_heap.hpp>
#include <boost/heap/pairing_heap.hpp>
#include <boost/heap/skew_heap.hpp>
#endif

#include "pq.hh"
#include "rec.hh"

using namespace std;

int run( char * fin )
{
  FILE *fdi = fopen( fin, "r" );

  struct stat st;
  fstat( fileno( fdi ), &st );
  size_t nrecs = st.st_size / REC_BYTES;

  vector<Rec> recs( nrecs );
  setup_value_storage( nrecs );

  auto t1 = chrono::high_resolution_clock::now();

  // read
  for ( uint64_t i = 0; i < nrecs; i++ ) {
    recs[i].read( fdi );
  }
  fclose( fdi );
  auto t2 = chrono::high_resolution_clock::now();

  // priority_queue choice
#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
  // boost::heap::priority_queue<Rec> pq;
  boost::heap::d_ary_heap<Rec, boost::heap::arity<6>> pq;
  // boost::heap::binomial_heap<Rec> pq;
  // boost::heap::fibonacci_heap<Rec> pq;
  // boost::heap::pairing_heap<Rec> pq;
  // boost::heap::skew_heap<Rec> pq;
#else
  mystl::priority_queue<Rec> pq;
#endif

  size_t size = nrecs / 5;

  // load -- priority-queue
  pq.reserve( size + 1 );
  for ( uint64_t i = 0; i < size; i++ ) {
    pq.push( recs[i] );
  }
  auto t3 = chrono::high_resolution_clock::now();

  // push + pop
  for ( uint64_t i = size; i < nrecs; i++ ) {
    if ( recs[i] < pq.top() ) {
      pq.push( recs[i] );
      pq.pop();
    }
  }
  auto t4 = chrono::high_resolution_clock::now();

  // // pop
  // for ( uint64_t i = 0; i < pq.size(); i++ ) {
  //   pq.pop();
  // }
  // auto t5 = chrono::high_resolution_clock::now();

  // stats
  auto t21 = chrono::duration_cast<chrono::milliseconds>( t2 - t1 ).count();
  auto t32 = chrono::duration_cast<chrono::milliseconds>( t3 - t2 ).count();
  auto t43 = chrono::duration_cast<chrono::milliseconds>( t4 - t3 ).count();
  auto t41 = chrono::duration_cast<chrono::milliseconds>( t4 - t1 ).count();

  // auto t54 = chrono::duration_cast<chrono::milliseconds>( t5 - t4 ).count();
  // auto t51 = chrono::duration_cast<chrono::milliseconds>( t5 - t1 ).count();

  cout << "Read     took " << t21 << "ms" << endl;
  cout << "PQ-load  took " << t32 << "ms" << endl;
  cout << "PQ-mixed took " << t43 << "ms" << endl;
  // cout << "PQ-pop   took " << t54 << "ms" << endl;
  cout << "Total    took " << t41 << "ms" << endl;

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
    cerr << e.what() << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

