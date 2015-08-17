/**
 * Test using a chunking strategy to overlap IO and sorting. Here we use a
 * vector for the storage containers.
 */
#include <sys/stat.h>
#include <iostream>
#include <vector>

#include "timestamp.hh" 
#include "util.hh"
#include "record.hh"

#include "r.hh"

using namespace std;

vector<R> scan( char * buf, size_t nrecs, size_t size, const R & after )
{
  auto t0 = time_now();
  tdiff_t tm = 0, ts = 0, tl = 0;
  size_t r1x = size / 4;

  vector<R> r1, r2, r3;
  r1.reserve( r1x );
  r2.reserve( size );

  for ( uint64_t i = 0 ; i < nrecs; ) {
    for ( ; r1.size() < r1x and i < nrecs; i++ ) {
      const unsigned char * r = (const unsigned char *) buf + Rec::SIZE * i;
      if ( after.compare( r, i ) < 0 ) {
        r1.emplace_back( r, i );
      }
      if ( after.compare( r, i ) < 0 ) {
        if ( r2.size() < size ) {
          r1.emplace_back( r, i );
        } else if ( r2.back().compare( r, i ) > 0 ) {
          r1.emplace_back( r, i );
        }
      }
    }
    
    if ( r1.size() > 0 ) {
      auto ts1 = time_now();
      rec_sort( r1.begin(), r1.end() );
      ts += time_diff<ms>( ts1 );
      r3.resize( min( size, r1.size() + r2.size() ) );
      tm += move_merge( r1, r2, r3 );
      swap( r2, r3 );
      r1.clear();
      tl = time_diff<ms>( ts1 );
    }
  }
  auto t1 = time_now();

  cout << "total, " << time_diff<ms>( t1, t0 ) << endl;
  cout << "sort , " << ts << endl;
  cout << "merge, " << tm << endl;
  cout << "last , " << tl << endl;
  
  return r2;
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
  size_t split = 10;
  size_t chunk = nrecs / split;
  cout << "size, " << nrecs << endl;
  cout << "cunk, " << chunk << endl;
  cout << endl;

  // starting record
  R after;
  memset( after.key_, 0x00, Rec::KEY_LEN );

  // scan file
  auto t1 = time_now();
  for ( uint64_t i = 0; i < split; i++ ) {
    auto r = scan( buf, nrecs, chunk, after );
    after = move( r[chunk - 1] );
    cout << "last: " << str_to_hex( after.key_, Rec::KEY_LEN ) << endl;
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

