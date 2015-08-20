/**
 * Test using a chunking strategy to overlap IO and sorting. Here we use an
 * array for the storage container.
 */
#include <sys/stat.h>
#include <iostream>

#include "file.hh"
#include "linux_compat.hh"
#include "overlapped_rec_io.hh"
#include "timestamp.hh" 
#include "util.hh"
#include "record.hh"

/* We can use a move or copy strategy -- the copy is actaully a little better
 * as we play some tricks to ensure we reuse allocations as much as possible.
 */
#define USE_MOVE 0

using namespace std;

// using RR = R;
using RR = RecordS;

RR * scan( OverlappedRecordIO<Rec::SIZE> & rio, size_t size, const RR & after )
{
  auto t0 = time_now();
  tdiff_t tm = 0, ts = 0, tl = 0;

  rio.rewind();

  size_t r1s = 0, r2s = 0;
  size_t r1x = max( min( (size_t) 1572864, size / 6 ), (size_t) 1 );
  RR * r1 = new RR[r1x];
  RR * r2 = new RR[size];
  RR * r3 = new RR[size];

  uint64_t i = 0;
  for ( bool run = true; run; ) {
    for ( r1s = 0; r1s < r1x; i++ ) {
      const uint8_t * r = (const uint8_t *) rio.next_record();
      if ( r == nullptr ) {
        run = false;
        break;
      }
      if ( after.compare( r, i ) < 0 ) {
        if ( r2s < size ) {
          r1[r1s++].copy( r, i );
        } else if ( r2[size - 1].compare( r, i ) > 0 ) {
          r1[r1s++].copy( r, i );
        }
      }
    }
    
    if ( r1s > 0 ) {
      auto ts1 = time_now();
      rec_sort( r1, r1 + r1s );
      ts += time_diff<ms>( ts1 );
#if USE_MOVE == 1
      tm += move_merge( r1, r1 + r1s, r2, r2 + r2s, r3, r3 + size );
#else
      tm += copy_merge( r1, r1 + r1s, r2, r2 + r2s, r3, r3 + size, true );
#endif
      swap( r2, r3 );
      r2s = min( size, r1s + r2s );
      r1s = 0;
      tl = time_diff<ms>( ts1 );
    }
  }
  auto t1 = time_now();

#if USE_MOVE == 0
  // r3 and r2 share storage cells, so clear to nullptr first.
  // TODO: arena or other stratergy may be better here.
  for ( uint64_t i = 0; i < size; i++ ) {
    r3[i].val_ = nullptr;
  }
#endif
  delete[] r3;
  delete[] r1;

  cout << "total, " << time_diff<ms>( t1, t0 ) << endl;
  cout << "sort , " << ts << endl;
  cout << "merge, " << tm << endl;
  cout << "last , " << tl << endl;
  
  return r2;
}

void run( char * fin )
{
  // open file
  File file( fin, O_RDONLY | O_DIRECT );
  OverlappedRecordIO<Rec::SIZE> rio( file );

  // stats
  size_t nrecs = file.size() / Rec::SIZE;
  size_t rounds = 10;
  size_t split = 10;
  size_t chunk = nrecs / split;
  cout << "size, " << nrecs << endl;
  cout << "cunk, " << chunk << endl;
  cout << endl;

  // starting record
  RR after( Rec::MIN );

  // scan file
  auto t1 = time_now();
  for ( uint64_t i = 0; i < rounds; i++ ) {
    auto r = scan( rio, chunk, after );
    after = move( r[chunk - 1] );
    cout << "last: " << str_to_hex( after.key_, Rec::KEY_LEN ) << endl;
    delete[] r;
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

