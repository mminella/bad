/**
 * Test program. Takes gensort file as input and reads whole thing into memory,
 * sorting using C++ algorithm sort implementation.
 */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <vector>

#include "exception.hh"
#include "util.hh"

#include "record.hh"

using namespace std;

int run( int argc, char * argv[] );

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc != 3 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) + " [file] [out]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    run( argc, argv );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int run( int argc, char * argv[] )
{
  // startup
  sanity_check_env( argc );
  check_usage( argc, argv );

  FILE *fdi = fopen( argv[1], "r" );
  FILE *fdo = fopen( argv[2], "w" );

  vector<Record> recs{};
  // PERF: avoid copying partial vectors
  recs.reserve( 10485700 );

  auto t1 = chrono::high_resolution_clock::now();

  // read
  char r[Record::SIZE];

  for ( uint64_t i = 0;; i++ ) {
    size_t n = fread( r, Record::SIZE, 1, fdi );
    if ( n == 0 ) {
      break;
    }
    // PERF: Avoid copy into array, construct in-place.
    recs.emplace_back( r, i, true );
  }
  fclose( fdi );
  auto t2 = chrono::high_resolution_clock::now();

  // sort
  sort( recs.begin(), recs.end() );
  auto t3 = chrono::high_resolution_clock::now();

  // write
  for ( auto & r : recs ) {
    fwrite( r.str( Record::NO_LOC ).c_str(), Record::SIZE, 1, fdo );
  }
  fflush( fdo );
  fsync( fileno( fdo ) );
  fclose( fdo );
  auto t4 = chrono::high_resolution_clock::now();

  // stats
  auto t21 = chrono::duration_cast<chrono::milliseconds>( t2 - t1 ).count();
  auto t32 = chrono::duration_cast<chrono::milliseconds>( t3 - t2 ).count();
  auto t43 = chrono::duration_cast<chrono::milliseconds>( t4 - t3 ).count();
  auto t41 = chrono::duration_cast<chrono::milliseconds>( t4 - t1 ).count();

  cout << "Read  took " << t21 << "ms" << endl;
  cout << "Sort  took " << t32 << "ms" << endl;
  cout << "Write took " << t43 << "ms" << endl;
  cout << "Total took " << t41 << "ms" << endl;

  return EXIT_SUCCESS;
}

