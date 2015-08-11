/**
 * Basic index building test.
 *
 * - Uses BufferedIO file IO.
 * - Uses custom record type.
 * - Uses STL vector + sort
 */
#include <sys/stat.h>

#include <chrono>
#include <iostream>
#include <vector>

#include "rec.hh"

using namespace std;

void run( char * fin )
{
  FILE *fdi = fopen( fin, "r" );

  struct stat st;
  fstat( fileno( fdi ), &st );
  size_t nrecs = st.st_size / 100;

  vector<RecordLoc> recs( nrecs );
  recs.reserve( nrecs );
  auto t1 = chrono::high_resolution_clock::now();

  // read
  char r[100];
  for ( uint64_t i = 0; i < nrecs; i++ ) {
    fread( r, 100, 1, fdi );
    recs.emplace_back( r, i * 100 + 10 );
  }
  auto t2 = chrono::high_resolution_clock::now();

  // sort
#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
  string_sort( recs.begin(), recs.end() );
#else
  sort( recs.begin(), recs.end() );
#endif
  auto t3 = chrono::high_resolution_clock::now();

  // stats
  auto t21 = chrono::duration_cast<chrono::milliseconds>( t2 - t1 ).count();
  auto t32 = chrono::duration_cast<chrono::milliseconds>( t3 - t2 ).count();
  auto t31 = chrono::duration_cast<chrono::milliseconds>( t3 - t1 ).count();

  cout << "Read  took " << t21 << "ms" << endl;
  cout << "Sort  took " << t32 << "ms" << endl;
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
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

