/**
 * Test using a chunking strategy to overlap IO and sorting. Here we use an
 * array for the storage container.
 */
#include <sys/stat.h>
#include <memory>
#include <iostream>

#include "record.hh"
#include "timestamp.hh"
#include "util.hh"

#include "config.h"
#include "merge_wrapper.hh"

/* We can use a move or copy strategy -- the copy is actaully a little better
 * as we play some tricks to ensure we reuse allocations as much as possible.
 * With copy we use `size + r1x` value memory, but with move, we use up to
 * `2.size + r1x`. */
#define USE_COPY 1

/* We can reuse our sort+merge buffers for a big win! Be careful though, as the
 * results returned by scan are invalidate when you next call scan. */
#define REUSE_MEM 1

/* Use the Intel TBB parallel sort?  */
#define TBB_PARALLEL_MERGE 1

using namespace std;
using RR = RecordS;

tdiff_t tmm = 0, tss = 0;
size_t gconsd = 0, gsorts = 0, gmergs = 0;

pair<uint64_t, uint64_t>
filter( const char * const buf, size_t nrecs, RR * r1, size_t size,
        uint64_t loc, const RR & after, const RR * const curMin )
{
  bool haveMin = curMin != nullptr;
  RR mmin;
  if ( haveMin ) {
    mmin = *curMin;
  }

  uint64_t i;
  for ( i = 0; i < size and loc < nrecs; loc++ ) {
    const unsigned char * r = (const unsigned char *) buf + Rec::SIZE * loc;
    RecordPtr next( r, loc );
    if ( after < next ) {
      if ( !haveMin ) {
        r1[i++].copy( next );
      } else if ( mmin.compare( r, i ) > 0 ) {
        r1[i++].copy( next );
      }
    }
  }
  return {i, loc};
}

RR * scan( const char * const buf, size_t nrecs, size_t size, const RR & after,
           RR * r1, RR * r2, RR * r3, size_t r1x )
{
  auto t0 = time_now();
  tdiff_t tm = 0, ts = 0, tl = 0;
  size_t r2s = 0, considered = 0, sorts = 0, merges = 0;
  RR * curMin = nullptr;

  for ( uint64_t loc = 0 ; loc < nrecs; ) {
    auto ix = filter( buf, nrecs, r1, r1x, loc, after, curMin );
    size_t r1s = ix.first; loc = ix.second;

    if ( r1s > 0 ) {
      considered += r1s;
      // SORT
      auto ts1 = time_now();
      rec_sort( r1, r1 + r1s );
      ts += time_diff<ms>( ts1 );
      sorts++;

      // MERGE
#if TBB_PARALLEL_MERGE == 1 && USE_COPY == 1
      tm += meth1_pmerge_copy( r1, r1+r1s, r2, r2+r2s, r3, r3+size );
#elif TBB_PARALLEL_MERGE == 1
      tm += meth1_pmerge_move( r1, r1+r1s, r2, r2+r2s, r3, r3+size );
#elif USE_COPY == 1
      tm += meth1_merge_copy( r1, r1 + r1s, r2, r2 + r2s, r3, r3 + size );
#else
      tm += meth1_merge_move( r1, r1 + r1s, r2, r2 + r2s, r3, r3 + size );
#endif
      merges++;

      // PREP
      swap( r2, r3 );
      r2s = min( size, r1s + r2s );
      tl = time_diff<ms>( ts1 );

      // NEW MIN
      if ( r2s == size ) {
        curMin = &r2[size - 1];
      }
    }
  }
  auto t1 = time_now();

  cout << "total, " << time_diff<ms>( t1, t0 ) << endl;
  cout << "sort , " << ts << endl;
  cout << "merge, " << tm << endl;
  cout << "last , " << tl << endl;

  gconsd += considered;
  gsorts += sorts;
  gmergs += merges;
  tss += ts;
  tmm += tm;

  return r2;
}

RR * scan( char * buf, size_t nrecs, size_t size, const RR & after )
{
  if ( size == 0 ) {
    return nullptr;
  }

#if REUSE_MEM == 1
  // our globals
  static size_t r1x = 0, r2x = 0;
  static RR *r1 = nullptr, *r2 = nullptr, *r3 = nullptr;

  // (re-)setup global buffers if size isn't correct
  if ( r2x != size ) {
    if ( r2x > 0 ) {
      delete[] r1;
      delete[] r2;
      delete[] r3;
    }
    r1x = max( size / 4, (size_t) 1 );
    r2x = size;
    r1 = new RR[r1x];
    r2 = new RR[size];
    r3 = new RR[size];
  }
#else /* !REUSE_MEM */
  size_t r1x = max( size / 4, (size_t) 1 );
  RR * r1 = new RR[r1x];
  RR * r2 = new RR[size];
  RR * r3 = new RR[size];
#endif

  RR * rr = scan( buf, nrecs, size, after, r1, r2, r3, r1x );
  if ( rr == r3 ) {
    swap( r2, r3 );
  }

#if REUSE_MEM == 0
#if USE_COPY == 1
  // r3 and r2 share storage cells, so clear to nullptr first.
  for ( uint64_t i = 0; i < size; i++ ) {
    r3[i].val_ = nullptr;
  }
#endif
  delete[] r3;
  delete[] r1;
#endif

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
  unique_ptr<char> buf( new char[st.st_size] );
  for ( int nr = 0; nr < st.st_size; ) {
    auto r = fread( buf.get(), st.st_size - nr, 1, fdi );
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
  RR after( Rec::MIN );

  // scan file
  auto t1 = time_now();
  for ( uint64_t i = 0; i < split; i++ ) {
    auto r = scan( buf.get(), nrecs, chunk, after );
    after.copy( r[chunk - 1] );
    cout << "last: " << after << endl;
#if REUSE_MEM == 0
    delete[] r;
#endif
  }
  cout << endl << "total: " << time_diff<ms>( t1 ) << endl;
  cout << "sort : " << tss << endl;
  cout << "merge: " << tmm << endl;
  cout << "--------------------" << endl;
  cout << "consd, " << gconsd << endl;
  cout << "sorts, " << gsorts << endl;
  cout << "mergs, " << gmergs << endl;
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

