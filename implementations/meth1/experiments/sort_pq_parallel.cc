/**
 * Test performance of priority_queue's.
 *
 * - Uses C file IO.
 * - Uses own Record struct.
 * - Use C++ std::priority_queue + std::vector.
 * - Uses Intel TBB for having two threads insert concurrently into PQ.
 */
#include <sys/stat.h>
#include <iostream>

#include "record.hh"
#include "timestamp.hh" 

#include "pq.hh"

#include "config.h"

#ifdef HAVE_TBB_MUTEX_H
#include "tbb/mutex.h"
#include "tbb/spin_rw_mutex.h"
#include "tbb/parallel_invoke.h"
#endif

using namespace std;

using RR = RecordS;
using PQ = mystl::priority_queue<RR>;

#ifdef HAVE_TBB_MUTEX_H
using MX= tbb::spin_rw_mutex;
MX rw_mutex;
#endif

void insert( PQ & pq, char * buf, size_t start, size_t nrecs, size_t size, const RR & after )
{
  for ( uint64_t i = start; i < nrecs; i++ ) {
    const char * r = buf + Rec::SIZE * i;
    RecordPtr next( r, i );
    if ( next > after ) {
#ifdef HAVE_TBB_MUTEX_H
      MX::scoped_lock lock( rw_mutex, false );
#endif
      if ( pq.size() < size ) {
#ifdef HAVE_TBB_MUTEX_H
        lock.upgrade_to_writer();
#endif
        pq.emplace( r, i );
      } else if ( next < pq.top() ) {
#ifdef HAVE_TBB_MUTEX_H
        lock.upgrade_to_writer();
#endif
        pq.emplace( r, i );
        pq.pop();
      }
    }
  }
}

vector<RR> scan( char * buf, size_t nrecs, size_t size, const RR & after )
{
  auto t0 = time_now();
  PQ pq{nrecs / 10 + 1};

  auto in1 = [&]{
    insert( pq, buf, 0, nrecs/2, size, after );
  };
  auto in2 = [&]{
    insert( pq, buf, nrecs/2, nrecs, size, after );
  };

#ifdef HAVE_TBB_MUTEX_H
  tbb::parallel_invoke( in1, in2 );
#else
  in1(); in2();
#endif

  auto t1 = time_now();

  vector<RR> vrecs = move( pq.container() );
  rec_sort( vrecs.begin(), vrecs.end() );

  cout << "pq: " << time_diff<ms>( t1, t0 ) << endl;
  return vrecs;
}

void run( char * fin )
{
  // open file
  FILE *fdi = fopen( fin, "r" );
  struct stat st;
  fstat( fileno( fdi ), &st );
  size_t nrecs = st.st_size / Rec::SIZE;

  // read file into memory
  auto t0 = time_now();
  char * buf = new char[st.st_size];
  for ( int nr = 0; nr < st.st_size; ) {
    auto r = fread( buf, st.st_size - nr, 1, fdi );
    if ( r <= 0 ) {
      break;
    }
    nr += r;
  }
  cout << "read , " << time_diff<ms>( t0 ) << endl;
  cout << endl;
  
  // stats
  cout << "size, " << nrecs << endl;
  cout << "cunk, " << nrecs / 10 << endl;
  cout << endl;

  // scan file
  auto t1 = time_now();
  RR after( Rec::MIN );
  for ( uint64_t i = 0; i < 10; i++ ) {
    auto recs = scan( buf, nrecs, nrecs / 10, after );
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



