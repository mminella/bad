/**
 * Adaptation of `sort_overlap_io` to use channels as the synchronization
 * primitive.
 *
 * - Uses C file IO + seperate reader thread (sync with channel).
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

#include "channel.hh"

#include "config.h"

#include "rec.hh"

#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
#include <boost/sort/spreadsort/string_sort.hpp>
using namespace boost::sort::spreadsort;
#endif

using namespace std;

class RecChunkIO
{
public:
  static constexpr size_t CHUNK = 100000;
  using time_point = chrono::high_resolution_clock::time_point;

private:
  FILE * f_;
  vector<Rec> & recs_;
  size_t nrecs_;

  thread reader_;
  Channel<size_t> chn_;
  time_point finished_;

  void read_file( void )
  {
    for ( uint64_t i = 0; i < nrecs_; i++ ) {
      recs_[i].read( f_ );
      if ( i != 0 && i % CHUNK == 0 ) {
        chn_.send( i );
      }
    }
    chn_.send( nrecs_ );
    finished_ = chrono::high_resolution_clock::now();
  }

public:
  RecChunkIO( FILE * f, vector<Rec> & recs, size_t nrecs )
    : f_{f}, recs_{recs}, nrecs_{nrecs}, reader_{}, chn_{10}, finished_{}
  {
    recs_ = vector<Rec>( nrecs_ );
    setup_value_storage( nrecs_ );
  };

  RecChunkIO( const RecChunkIO & r ) = delete;
  RecChunkIO( RecChunkIO && r ) = delete;
  RecChunkIO & operator=( const RecChunkIO & r ) = delete;
  RecChunkIO & operator=( RecChunkIO && r ) = delete;

  ~RecChunkIO()
  {
    fclose( f_ );
    if ( reader_.joinable() ) { reader_.join(); }
  }

  void start( void )
  {
    reader_ = thread( &RecChunkIO::read_file,  this );
  }

  time_point finished( void )
  {
    reader_.join();
    return finished_;
  }

  size_t next( void )
  {
    return chn_.recv();
  }
};

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
  vector<Rec> recs;
  RecChunkIO chunkIO( fdi, recs, nrecs );
  chunkIO.start();

  // sort -- overlapping
  chrono::high_resolution_clock::time_point ffs, ffe, t2a;
  t2a = chrono::high_resolution_clock::now();
  size_t sorted_recs = 0;
  while ( sorted_recs < nrecs ) {
    size_t next = chunkIO.next();
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
  auto t2 = chunkIO.finished();
  auto t3 = chrono::high_resolution_clock::now();

  // write
  for ( uint64_t i = 0; i < nrecs; i++ ) {
    recs[i].write( fdo );
  }
  fflush( fdo );
  fsync( fileno( fdo ) );
  fclose( fdo );
  auto t4 = chrono::high_resolution_clock::now();

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


