#include <atomic>
#include <functional>
#include <thread>

#include "config.h"
#ifdef HAVE_TBB_TASK_GROUP_H
#include "tbb/task_group.h"
#endif

#include "exception.hh"
#include "sync_print.hh"
#include "timestamp.hh"

#include "record.hh"

#include "sort.hh"

using namespace std;

size_t odirectAlignSize( size_t len )
{
  // Linux O_DIRECT requires read buffer be both block aligned and a multiple
  // of blocks. Urgh.
  if ( len % IODevice::ODIRECT_ALIGN != 0 ) {
    len = len / IODevice::ODIRECT_ALIGN + 1;
    len *= IODevice::ODIRECT_ALIGN;
  }
  return len;
}

char * allocBucket( size_t len )
{
  char * buf;
  SystemCall( "posix_memalign", posix_memalign( (void **) &buf,
    IODevice::ODIRECT_ALIGN, len ) );
  return buf;
}

BucketSorter::BucketSorter( const ClusterMap & cluster, uint16_t bkt )
  : cluster_{cluster}
  , bkt_{bkt}
  , len_{0}
  , buf_{nullptr}
{}

BucketSorter::BucketSorter( BucketSorter && other )
  : cluster_{other.cluster_}
  , bkt_{other.bkt_}
  , len_{other.len_}
  , buf_{other.buf_}
{
  other.buf_ = nullptr;
}

void BucketSorter::loadBucket( void )
{
  File in( cluster_.bucket_path( bkt_ ), O_RDONLY, File::DIRECT );
  len_ = in.size();
  if ( len_ % Rec::SIZE != 0 ) {
    throw runtime_error( "Bucket not a multiple of record size" );
  }
  size_t blen = odirectAlignSize( len_ );
  buf_ = allocBucket( blen );
  in.read( buf_, blen );
}

void BucketSorter::sortBucket( void )
{
  RecordString * recs = (RecordString *) buf_;
  rec_sort( recs, recs + ( len_ / Rec::SIZE ) );
}

void BucketSorter::saveBucket( void )
{
  File out( cluster_.sorted_bucket_path( bkt_ ),
    O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR );
  out.write_all( buf_, len_ );
  out.fsync();
  // PERF: Reuse buffes?
  free( buf_ );
}

// Handle sorting all buckets on a single disk
void sortDisk( const ClusterMap & cluster, size_t diskID )
{
  // filter out buckets for my disk
  vector<BucketSorter> bsorters;
  for ( auto bkt : cluster.myBuckets() ) {
    if ( cluster.bucket_disk( bkt ) == diskID ) {
      bsorters.emplace_back( cluster, bkt );
    }
  }

  if ( bsorters.size() == 0 ) {
    return;
  }

  tdiff_t tsort = 0, tsave = 0;
  atomic<tdiff_t> tload;
  tload.store( 0 );

#ifdef HAVE_TBB_TASK_GROUP_H
  // Overlap sorting and saving of one bucket, with reading of next
  tbb::task_group tg;

  {
    // load first bucket
    auto t0 = time_now();
    bsorters[0].loadBucket();
    tload += time_diff<ms>( t0 );
  }

  for ( size_t i = 1; i < bsorters.size(); i++ ) {
    // wait for load of i - 1 to finish
    tg.wait();
    // start loading of i
    tg.run([&bsorters, i, &tload]() {
      auto t0 = time_now();
      bsorters[i].loadBucket();
      tload += time_diff<ms>( t0 );
    });

    // sort and save i
    auto t0 = time_now();
    bsorters[i-1].sortBucket();
    auto t1 = time_now();
    bsorters[i-1].saveBucket();
    auto t2 = time_now();

    // timings
    tsort += time_diff<ms>( t1, t0 );
    tsave += time_diff<ms>( t2, t1 );
  }
  tg.wait();

  {
    // sort + save last bucket
    auto t0 = time_now();
    bsorters.back().sortBucket();
    auto t1 = time_now();
    bsorters.back().saveBucket();
    auto t2 = time_now();

    // timings
    tsort += time_diff<ms>( t1, t0 );
    tsave += time_diff<ms>( t2, t1 );
  }
#else
  for ( auto & bs : bsorters ) {
    auto t0 = time_now();
    bs.loadBucket();
    auto t1 = time_now();
    bs.sortBucket();
    auto t2 = time_now();
    bs.saveBucket();
    auto t3 = time_now();

    tload += time_diff<ms>( t1, t0 );
    tsort += time_diff<ms>( t2, t1 );
    tsave += time_diff<ms>( t3, t2 );
  }
#endif

  print( "sort-disk", timestamp<ms>(), diskID, tload, tsort, tsave );
}

Sorter::Sorter( const ClusterMap & cluster )
{
  vector<thread> diskSorters;
  for ( size_t i = 0; i < cluster.disks(); i++ ) {
    diskSorters.emplace_back( sortDisk, ref( cluster ), i );
  }
  for ( auto & ds : diskSorters ) {
    ds.join();
  }
}
