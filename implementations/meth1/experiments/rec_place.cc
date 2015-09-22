/**
 * Test how quickly we can just read from disk, filtering some keys and storing
 * into memory.
 */
#include <stdlib.h>
#include <sys/stat.h>

#include <iostream>
#include <numeric>

#include "file.hh"
#include "overlapped_rec_io.hh"
#include "linux_compat.hh"
#include "record.hh"
#include "timestamp.hh" 
#include "util.hh"

#include "config.h"
#ifdef HAVE_TBB_TASK_GROUP_H
#include "tbb/task_group.h"
#endif

using namespace std;

using RR = Record;

uint64_t scan_inmem( char * buf, uint64_t nrecs, RR * out, const RR * after )
{
  size_t zrecs = 0;
  for ( size_t i = 1; i < nrecs; i++ ) {
    RecordPtr next( buf + Rec::SIZE * i );
    if ( *after < next ) {
      out[zrecs++].copy( next );
    }
  }
  return zrecs;
}

void test_end( tpoint_t t, uint64_t nrecs, uint64_t zrecs )
{
  auto tt = time_diff<ms>( t );
  if ( tt != 0 ) {
    uint64_t nbs = nrecs * Rec::SIZE / tt * 1000 / 1024 / 1024;
    uint64_t zbs = zrecs * Rec::SIZE / tt * 1000 / 1024 / 1024;
    cout << "time , " << tt << endl;
    cout << "nrecs, " << nrecs << ", " << nbs << endl;
    cout << "zrecs, " << zrecs << ", " << zbs << endl;
    cout << endl;
  }
}

void run_inmem_seq( char * rbuf, uint64_t nrecs )
{
  cout << "sequential" << endl;

  RR * recs = new RR[nrecs];
  RR after( rbuf );
  auto t0 = time_now();

  auto zrecs = scan_inmem( rbuf, nrecs, recs, &after );

  test_end( t0, nrecs, zrecs );
  delete[] recs;
}

void run_inmem_par( char * rbuf, uint64_t nrecs, size_t p )
{
  cout << "parallel, " << p << endl;
#ifdef HAVE_TBB_TASK_GROUP_H
  tbb::task_group tg;
#endif

  RR * recs = new RR[nrecs];
  RR after( rbuf );
  auto t0 = time_now();

  uint64_t * zrecs_i = new uint64_t[p];
  for ( size_t i = 0; i < p; i++ ) {
    char * buf_i = rbuf + (nrecs / p * Rec::SIZE) * i;
    RR * recs_i = recs + (nrecs / p) * i;
#ifdef HAVE_TBB_TASK_GROUP_H
    tg.run( [=]() {
      zrecs_i[i] = scan_inmem( buf_i, nrecs / p, recs_i, &after );
    } );
#else
    zrecs_i[i] = scan_inmem( buf_i, nrecs / p, recs_i, &after );
#endif
  }
#ifdef HAVE_TBB_TASK_GROUP_H
  tg.wait();
#endif
  uint64_t zrecs = accumulate( zrecs_i, zrecs_i + p, 0 );

  test_end( t0, nrecs, zrecs );
  delete[] recs;
}

void run_inmem( char * fin )
{
  // 1) OPEN FILE
  FILE *fdi = fdopen( open( fin, O_RDONLY, true ), "r" );
  struct stat st;
  fstat( fileno( fdi ), &st );
  size_t nrecs = st.st_size / Rec::SIZE;

  // 2) READ FILE INTO MEMORY
  auto tr = time_now();
  char * rbuf;
  if ( posix_memalign( (void **) &rbuf, 4096, st.st_size ) < 0 ) {
    cerr << "error using posix_memalign" << endl;
  }
  for ( int nr = 0; nr < st.st_size; ) {
    auto r = fread( rbuf, st.st_size - nr, 1, fdi );
    if ( r <= 0 ) {
      break;
    }
    nr += r;
  }
  auto tt = time_diff<ms>( tr );
  uint64_t mbs = st.st_size * 1000 / tt / 1024 / 1024;
  cout << "read, " << tt << ", " << mbs << endl;
  cout << endl;

  run_inmem_seq( rbuf, nrecs );

  
  for ( size_t p = 2; p <= thread::hardware_concurrency(); p++ ) {
    run_inmem_par( rbuf, nrecs, p );
  }
}

void run_overlap( char * fin )
{
  // open file
  File file( fin, O_RDONLY );
  OverlappedRecordIO<Rec::SIZE> rio( file );
  size_t nrecs = file.size() / Rec::SIZE;

  rio.rewind();

  // use first record as a filter
  auto * r = rio.next_record();
  RR after( r );

  // filter file
  auto t0 = time_now();
  RR * recs = new RR[nrecs];
  size_t zrecs = 0;
  for ( size_t i = 0; i < nrecs; i++ ) {
    r = rio.next_record();
    if ( r == nullptr ) {
      break;
    }
    RecordPtr next( r );
    if ( after < next ) {
      recs[zrecs++].copy( next );
    }
  }
  delete[] recs;

  auto tt = time_diff<ms>( t0 );
  uint64_t nbs = nrecs * Rec::SIZE / tt * 1000 / 1024 / 1024;
  uint64_t zbs = zrecs * Rec::SIZE / tt * 1000 / 1024 / 1024;

  cout << "time , " << tt << endl;
  cout << "nrecs, " << nrecs << ", " << nbs << endl;
  cout << "zrecs, " << zrecs << ", " << zbs << endl;
  cout << endl;
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
    // run_overlap( argv[1] );
    run_inmem( argv[1] );
  } catch ( const exception & e ) {
    cerr << e.what() << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

