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

class RecLoader
{
private:
  std::unique_ptr<File> file_;
  std::unique_ptr<RecIO> rio_;
  bool eof_;
  uint64_t loc_;

public:
  RecLoader( string fileName, int flags, bool odirect = false )
    : file_{new File( fileName, flags, odirect ? File::DIRECT : File::CACHED )}
    , rio_{new RecIO( *file_ )}
    , eof_{false}
    , loc_{0}
  {}

  /* no copy */
  RecLoader( const RecLoader & ) = delete;
  RecLoader & operator=( const RecLoader & ) = delete;

  /* allow move */
  RecLoader( RecLoader && other )
    : file_{std::move( other.file_ )}
    , rio_{std::move( other.rio_ )}
    , eof_{other.eof_}
    , loc_{other.loc_}
  {}

  RecLoader & operator=( RecLoader && other )
  {
    if ( this != &other ) {
      file_ = std::move( other.file_ );
      rio_ = std::move( other.rio_ );
      eof_ = other.eof_;
      loc_ = other.loc_;
    }
    return *this;
  }

  const File & file( void ) const noexcept { return *file_; }

  bool eof( void ) const noexcept { return eof_; }

  void rewind( void )
  {
    loc_ = 0;
    eof_ = false;
    rio_->rewind();
  }

  uint64_t
  filter( RR * r1, uint64_t size, const RR & after, const RR * const curMin )
  {
    if ( eof_ ) {
      return 0;
    }

    if ( curMin == nullptr ) {
      for ( uint64_t i = 0; i < size; loc_++ ) {
        const uint8_t * r = (const uint8_t *) rio_->next_record();
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
        const uint8_t * r = (const uint8_t *) rio_->next_record();
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

RR * scan( vector<RecLoader> & rios, size_t size, const RR & after )
{
  auto t0 = time_now();

  tbb::task_group tg;
  tdiff_t tm = 0, ts = 0, tl = 0;

  // setup loader processes
  for ( auto & rio : rios ) {
    rio.rewind();
  }
  uint64_t * r1s_i = new uint64_t[rios.size()];

  size_t r1s = 0, r2s = 0;
  const size_t r1x = max( min( (size_t) 1572864, size / 6 ), (size_t) 1 );
  const size_t r1x_i = r1x / rios.size();
  const RR * curMin = nullptr;
  RR * r1 = new RR[r1x];
  RR * r2 = new RR[size];
  RR * r3 = new RR[size];

  while ( true ) {
    // kick of all record loader
    uint64_t rio_i = 0;
    for ( auto & rio : rios ) {
      if ( not rio.eof() ) {
        tg.run( [&rio, rio_i, r1s_i, r1, r1x_i, &after, curMin]() {
          r1s_i[rio_i] =
            rio.filter( &r1[r1x_i * rio_i], r1x_i, after, curMin );
        } );
        rio_i++;
      }
    }
    tg.wait();

    // eof?
    if ( rio_i == 0 ) {
      break;
    }

    // move all loader results to be contiguous
    r1s = r1s_i[0];
    bool moving = false;
    for ( uint64_t i = 0; i < rio_i - 1; i++ ) {
      moving |= r1s_i[i] < r1x_i;
      if ( moving ) {
        uint64_t i1_start = r1x_i * (i + 1);
        uint64_t i1_end = i1_start + r1s_i[i + 1];
        move( &r1[i1_start], &r1[i1_end], &r1[r1s] );
      }
      r1s += r1s_i[i+1];
    }

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
  }
  auto t1 = time_now();
  delete[] r1s_i;

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
  cout << "files, " << fileNames.size() << endl;

  // open files
  vector<RecLoader> recio;
  for ( auto & fn : fileNames ) {
    recio.emplace_back( fn, O_RDONLY );
  }

  // ASSUMPTION: Files are same size

  // stats
  size_t rounds = 10;
  size_t split = 10;
  // size_t nrecs = file.size() / Rec::SIZE;
  size_t nrecs = accumulate( recio.begin(), recio.end(), 0,
    []( size_t res, RecLoader & f ) {
      return res + f.file().size() / Rec::SIZE;
    } );
  size_t chunk = nrecs / split;
  cout << "size, " << nrecs << endl;
  cout << "cunk, " << chunk << endl;
  cout << endl;

  // starting record
  RR after( Rec::MIN );

  // scan file
  auto t1 = time_now();
  for ( uint64_t i = 0; i < rounds; i++ ) {
    auto r = scan( recio, chunk, after );
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

