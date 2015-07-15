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

static constexpr size_t NRECS = 10485700;
static constexpr size_t REC_BYTES = NRECS * 100;

struct Rec
{
  char   key_[10];
  char * rec_;
  size_t loc_;

  Rec( char * s, size_t loc ) : rec_{s}, loc_{loc}
  {
    memcpy( key_, rec_, 10 );
  }

  /* Comparison */
  bool operator<( const Rec & b ) const
  {
    return compare( b ) < 0 ? true : false;
  }

  /* we compare on key first, and then on diskloc_ */
  int compare( const Rec & b ) const
  {
    int cmp = memcmp( key_, b.key_, 10 );
    if ( cmp == 0 ) {
      if ( loc_ < b.loc_ ) {
        cmp = -1;
      }
      if ( loc_ > b.loc_ ) {
        cmp = 1;
      }
    }
    return cmp;
  }
};

int run( int argc, char * argv[] )
{
  // startup
  check_usage( argc, argv );

  FILE *fdi = fopen( argv[1], "r" );
  FILE *fdo = fopen( argv[2], "w" );

  char * all_recs = new char[ REC_BYTES ];

  vector<Rec> recs{};
  recs.reserve( 10485700 );

  auto t1 = chrono::high_resolution_clock::now();

  // read
  char * r = all_recs;
  for ( uint64_t i = 0;; i++ ) {
    size_t n = fread( r, 100, 1, fdi );
    if ( n == 0 ) {
      break;
    }
    recs.emplace_back( r, i );
    r += 100;
  }
  fclose( fdi );
  auto t2 = chrono::high_resolution_clock::now();

  // sort
  sort( recs.begin(), recs.end() );
  auto t3 = chrono::high_resolution_clock::now();

  // write
  for ( auto & r : recs ) {
    fwrite( r.rec_, 100, 1, fdo );
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


