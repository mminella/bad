/**
 * Test using a chunking strategy to overlap IO and sorting. Here we use an
 * array for the storage container.
 */
#include <sys/stat.h>
#include <iostream>
#include <numeric>
#include <vector>
#include <utility>

#include "file.hh"
#include "linux_compat.hh"
#include "overlapped_rec_io.hh"
#include "record.hh"
#include "timestamp.hh" 
#include "util.hh"

#include "merge_wrapper.hh"

#include "config.h"
#ifdef HAVE_TBB_TASK_GROUP_H
#include "tbb/task_group.h"
#endif

/* We can use a move or copy strategy -- the copy is actaully a little better
 * as we play some tricks to ensure we reuse allocations as much as possible.
 */
#define USE_COPY 1

using namespace std;

using RR = RecordS;
using RecIO = OverlappedRecordIO<Rec::SIZE>;

class RecFilter
{
private:
  RecIO & rio_;
  bool eof_;
  uint64_t loc_;

public:
  RecFilter( RecIO & rio )
    : rio_{rio}, eof_{false}, loc_{0}
  {
    rewind();
  }

  bool eof( void ) const noexcept { return eof_; }

  void rewind( void )
  {
    eof_ = false;
    rio_.rewind();
  }

  uint64_t
  operator()( RR * r1, uint64_t size, const RR & after, const RR * const curMin )
  {
    if ( eof_ ) {
      return 0;
    }

    if ( curMin == nullptr ) {
      for ( uint64_t i = 0; i < size; loc_++ ) {
        const uint8_t * r = (const uint8_t *) rio_.next_record();
        if ( r == nullptr ) {
          eof_ = true;
          return i;
        }
        if ( after.compare( r, loc_ ) < 0 ) {
          r1[i++].copy( r, loc_ );
        }
      }
    } else {
      for ( uint64_t i = 0; i < size; loc_++ ) {
        const uint8_t * r = (const uint8_t *) rio_.next_record();
        if ( r == nullptr ) {
          eof_ = true;
          return i;
        }
        if ( after.compare( r, loc_ ) < 0 and
              curMin->compare( r, loc_ ) > 0 ) {
          r1[i++].copy( r, loc_ );
        }
      }
    }
    return size;
  }
};

RR * scan( vector<RecIO> & rios, size_t size, const RR & after )
{
  auto t0 = time_now();

  // tbb::task_group tg;
  tdiff_t tm = 0, ts = 0, tl = 0;
  RecFilter filter( rios[0] );

  size_t r1s = 0, r2s = 0;
  size_t r1x = max( min( (size_t) 1572864, size / 6 ), (size_t) 1 );
  RR * r1 = new RR[r1x];
  RR * r2 = new RR[size];
  RR * r3 = new RR[size];
  RR * curMin = nullptr;

  while ( true ) {
    r1s = filter( r1, r1x, after, curMin );
    
    if ( r1s > 0 ) {
      auto ts1 = time_now();
      rec_sort( r1, r1 + r1s );
      ts += time_diff<ms>( ts1 );
#if USE_COPY == 1
      tm += meth1_merge_copy( r1, r1 + r1s, r2, r2 + r2s, r3, r3 + size );
#else
      tm += meth1_merge_move( r1, r1 + r1s, r2, r2 + r2s, r3, r3 + size );
#endif
      swap( r2, r3 );
      r2s = min( size, r1s + r2s );
      tl = time_diff<ms>( ts1 );

      if ( r2s == size ) {
        curMin = &r2[size - 1];
      }
    }
    
    if ( filter.eof() ) {
      break;
    } else {
      r1s = 0;
    }
  }
  auto t1 = time_now();

#if USE_COPY == 1
  // r3 and r2 share storage cells, so clear to nullptr first.
  for ( uint64_t i = 0; i < size; i++ ) {
    r3[i].set_val( nullptr );
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

void run( vector<string> fileNames )
{
  // open files
  vector<File> files;
  vector<RecIO> rios;
  for ( auto & fn : fileNames ) {
    files.emplace_back( fn, O_RDONLY | O_DIRECT );
    rios.emplace_back( files.back() );
  }

  // ASSUMPTION: Files are same size

  // stats
  size_t rounds = 10;
  size_t split = 10;
  // size_t nrecs = file.size() / Rec::SIZE;
  size_t nrecs = accumulate( files.begin(), files.end(), 0,
    []( size_t res, File & f ) { return res + f.size() / Rec::SIZE; } );
  size_t chunk = nrecs / split;
  cout << "size, " << nrecs << endl;
  cout << "cunk, " << chunk << endl;
  cout << endl;

  // starting record
  RR after( Rec::MIN );

  // scan file
  auto t1 = time_now();
  for ( uint64_t i = 0; i < rounds; i++ ) {
    auto r = scan( rios, chunk, after );
    after = move( r[chunk - 1] );
    cout << "last: " << after << endl;
    delete[] r;
  }
  cout << endl << "total: " << time_diff<ms>( t1 ) << endl;
}

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc < 2 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) + " [file..]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    run( {argv+1, argv+argc} );
  } catch ( const exception & e ) {
    cerr << e.what() << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

