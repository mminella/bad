/**
 * Test performance of memcmp.
 *
 * - Uses own Record struct.
 * - Use C++ std::sort method + std::vector.
 */
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <system_error>
#include <vector>

#include "bench.hh"

#include "rec.hh"

#define REPEAT 3

using namespace std;

int run( char * fin )
{
  FILE *fdi = fopen( fin, "r" );

  struct stat st;
  fstat( fileno( fdi ), &st );
  size_t nrecs = st.st_size / REC_BYTES;

  if ( nrecs == 0 ) {
    throw runtime_error( "Empty file" );
  }

  vector<Rec> recs( nrecs );
  setup_value_storage( nrecs );

  auto t1 = chrono::high_resolution_clock::now();

  // read
  for ( uint64_t i = 0; i < nrecs; i++ ) {
    recs[i].read( fdi );
  }
  fclose( fdi );
  auto t2 = chrono::high_resolution_clock::now();

  // compare
  Rec r = recs.back();
  int less = 0;
  uint64_t b0 = bench_start();
  for ( uint64_t j = 0; j < REPEAT; j++ ) {
    for ( uint64_t i = 0; i < nrecs; i++ ) {
      less += r < recs[i];
    }
  }

  uint64_t b1 = bench_end();
  auto t3 = chrono::high_resolution_clock::now();

  cout << "Less: " << less / REPEAT << endl;

  uint64_t overhead = measure_bench_overhead();
  uint64_t bb = b1 - b0 - overhead;

  // stats
  auto t21 = chrono::duration_cast<chrono::milliseconds>( t2 - t1 ).count();
  auto t32 = chrono::duration_cast<chrono::milliseconds>( t3 - t2 ).count();

  uint64_t kbs = nrecs * KEY_BYTES * REPEAT  / 1024 / t32 * 1000;

  cout << "Comparisons  " << nrecs * REPEAT << endl;
  cout << "Read took    " << t21 << "ms" << endl;
  cout << "Cmp  took    " << t32 << "ms" << endl;
  cout << "Cmp  cycles  " << bb << endl;
  cout << "Cmp 1-1      " << bb / nrecs / REPEAT << endl;
  cout << "KB/s         " << kbs << endl;
  cout << "" << endl;
  cout << "No measure of overhead for addition in loop..." << endl;

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

