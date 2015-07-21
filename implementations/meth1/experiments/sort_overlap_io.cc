/**
 * Adaptation of `sort_boost` to overlap reading IO with sorting of the file.
 *
 * - Uses C file IO + seperate reader thread (sync with mutex + cv).
 * - Uses own Record struct.
 * - Use boost::spreadsort + std::vector.
 */
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "../config.h"

#include "rec.hh"

#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
#include <boost/sort/spreadsort/string_sort.hpp>
using namespace boost::sort::spreadsort;
#endif

using namespace std;

// read:sort chunk size
static constexpr size_t CHUNK = 100000;

// All records (key + ptr to value)
static vector<Rec> recs;

// overlapping chunks
static mutex mtx;
static condition_variable cv;
static size_t ready_recs = 0;
bool next_chunk_ready( void ) { return ready_recs != 0; }

// read timing
chrono::high_resolution_clock::time_point t2;

// read in the whole file
void read_all_recs( FILE * fdi, size_t nrecs )
{
  // setup space for data
  recs = vector<Rec>( nrecs );
  setup_value_storage( nrecs );

  // read all records
  for ( uint64_t i = 0; i < nrecs; i++ ) {
    recs[i].read( fdi );
    if ( i != 0 && i % CHUNK == 0 ) {
      unique_lock<mutex> lck (mtx);
      ready_recs = i;
      cv.notify_one();
    }
  }

  unique_lock<mutex> lck (mtx);
  ready_recs = nrecs;
  cv.notify_one();

  fclose( fdi );
  t2 = chrono::high_resolution_clock::now();
}

// has the reader thread retrieved the next chunk of data?
size_t get_next_chunk( void )
{
  unique_lock<mutex> lck (mtx);
  cv.wait( lck, next_chunk_ready );
  size_t r = ready_recs;
  ready_recs = 0;
  return r;
}

int run( char * fin, char * fout )
{
  FILE *fdi = fopen( fin, "r" );
  FILE *fdo = fopen( fout, "w" );

  auto t1 = chrono::high_resolution_clock::now();

  // get filesize
  struct stat st;
  fstat( fileno( fdi ), &st );
  size_t nrecs = st.st_size / REC_BYTES;

  // read
  thread read_thread ( read_all_recs,  fdi, nrecs  );

  // sort -- overlapping
  chrono::high_resolution_clock::time_point ffs, ffe;
  auto t2a = chrono::high_resolution_clock::now();
  size_t sorted_recs = 0;
  while ( sorted_recs < nrecs ) {
    size_t next = get_next_chunk();
    ffs = chrono::high_resolution_clock::now();
#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
    string_sort( recs.begin() + sorted_recs, recs.begin() + next );
#else
    sort( recs.begin() + sorted_recs, recs.begin() + next );
#endif
    inplace_merge( recs.begin(), recs.begin() + sorted_recs,
                   recs.begin() + next );
    sorted_recs = next;
    ffe = chrono::high_resolution_clock::now();
  }
  auto t3 = chrono::high_resolution_clock::now();

  // write
  for ( uint64_t i = 0; i < nrecs; i++ ) {
    recs[i].write( fdo );
  }
  fflush( fdo );
  fsync( fileno( fdo ) );
  fclose( fdo );
  auto t4 = chrono::high_resolution_clock::now();

  // sync read thread
  read_thread.join();

  // stats
  auto t21  = chrono::duration_cast<chrono::milliseconds>( t2 - t1 ).count();
  auto t32a = chrono::duration_cast<chrono::milliseconds>( t3 - t2a ).count();
  auto t31  = chrono::duration_cast<chrono::milliseconds>( t3 - t1 ).count();
  auto t43  = chrono::duration_cast<chrono::milliseconds>( t4 - t3 ).count();
  auto t41  = chrono::duration_cast<chrono::milliseconds>( t4 - t1 ).count();
  auto tff  = chrono::duration_cast<chrono::milliseconds>( ffe - ffs ).count();

  cout << "Read  took " << t21 << "ms" << endl;
  cout << "Sort  took " << t32a << "ms" << endl;
  cout << "R+S   took " << t31 << "ms" << endl;
  cout << "Final took " << tff << "ms" << endl;
  cout << "Write took " << t43 << "ms" << endl;
  cout << "Total took " << t41 << "ms" << endl;

  return EXIT_SUCCESS;
}

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc != 3 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) + " [file] [out]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    run( argv[1], argv[2] );
  } catch ( const exception & e ) {
    cerr << e.what() << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

