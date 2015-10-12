#include <atomic>
#include <functional>
#include <limits>
#include <thread>

#include "config.h"
#ifdef HAVE_TBB_TASK_GROUP_H
#include "tbb/task_group.h"
#endif

#include "exception.hh"
#include "socket.hh"
#include "sync_print.hh"
#include "timestamp.hh"

#include "record.hh"

#include "meth4_knobs.hh"
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

BucketSorter::~BucketSorter( void )
{
  freeBucket();
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
}

void BucketSorter::sendBucket( TCPSocket & sock, uint64_t records )
{
  auto t0 = time_now();
  print( "send-bucket", timestamp<ms>(), bkt_, min( len_, records * Rec::SIZE ) );
  sock.write_all( buf_, min( len_, records * Rec::SIZE ) );
  print( "sent-bucket", timestamp<ms>(), bkt_, time_diff<ms>( t0 ) );
}

void BucketSorter::freeBucket( void )
{
  if ( buf_ != nullptr ) {
    // PERF: Reuse buffes?
    free( buf_ );
    buf_ = nullptr;
  }
}

pair<uint64_t, bool> calculateOp( string op, string arg1 )
{
  if ( op == "all" ) {
    return make_pair( std::numeric_limits<uint64_t>::max(), false );
  } else if ( op == "nth" ) {
    return make_pair( atoll( arg1.data() ), false );
  } else if ( op == "first" ) {
    return make_pair( 1, false );
  } else if ( op == "all-client" ) {
    return make_pair( std::numeric_limits<uint64_t>::max(), true );
  } else if ( op == "nth-client" ) {
    return make_pair( atoll( arg1.data() ), true );
  } else if ( op == "first-client" ) {
    return make_pair( 1, true );
  } else {
    throw runtime_error( "Unknown operation: " + op );
  }
}

// Handle sorting all buckets on a single disk
void sortDisk( const ClusterMap & cluster, size_t diskID, string op,
               string arg1 )
{
  auto range = calculateOp( op, arg1 );
  bool toClient = range.second;
  uint64_t bktSize = cluster.bucketSizeAvg();

  // filter out buckets for my disk
  vector<BucketSorter> bsorters;
  size_t diskBuckets = 0;
  for ( auto bkt : cluster.myBuckets() ) {
    if ( cluster.bucket_disk( bkt ) == diskID ) {
      diskBuckets++;
      // check in sorting range
      if ( bkt * bktSize <= range.first ) {
        bsorters.emplace_back( cluster, bkt );
      }
    }
  }

  print( "sort-disk", timestamp<ms>(), diskID, diskBuckets, bsorters.size(),
    toClient );
  if ( bsorters.size() == 0 ) {
    return;
  }

  // connect to client if needed
  TCPSocket client;
  if ( toClient ) {
    print( "sending-to-client", cluster.client().to_string() );
    TCPSocket s( IPV4 );
    s.set_nodelay();
    s.set_send_buffer( Knobs::NET_SND_BUF );
    s.set_recv_buffer( Knobs::NET_RCV_BUF );
    s.connect( cluster.client() );
    client = move( s );
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
    // start loading of i
    tg.run([&bsorters, i, &tload]() {
      auto t0 = time_now();
      bsorters[i].loadBucket();
      tload += time_diff<ms>( t0 );
    });

    // sort i-1
    auto t0 = time_now();
    bsorters[i-1].sortBucket();
    auto t1 = time_now();

    // save i-1
    if ( toClient ) {
      tg.run([&client, &bsorters, i, bktSize, range]() {
        uint64_t bktLim = range.first - bsorters[i-1].id() * bktSize;
        bsorters[i-1].sendBucket( client, bktLim );
      });
    }
    bsorters[i-1].saveBucket();
    auto t2 = time_now();

    // timings
    tsort += time_diff<ms>( t1, t0 );
    tsave += time_diff<ms>( t2, t1 );

    // wait for load of i to finish, and i-1 to save
    tg.wait();
    bsorters[i-1].freeBucket();
  }

  {
    // sort + save last bucket
    auto t0 = time_now();
    bsorters.back().sortBucket();
    auto t1 = time_now();
    if ( toClient ) {
      tg.run([&client, &bsorters, bktSize, range]() {
        uint64_t bktLim = range.first - bsorters.back().id() * bktSize;
        bsorters.back().sendBucket( client, bktLim );
      });
    }
    bsorters.back().saveBucket();
    auto t2 = time_now();
    tg.wait();
    bsorters.back().freeBucket();

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
    if ( toClient ) {
      uint64_t bktLim = range.first - bs.id() * bktSize;
      bs.sendBucket( client, bktLim );
    }
    auto t3 = time_now();
    bs.freeBucket();

    tload += time_diff<ms>( t1, t0 );
    tsort += time_diff<ms>( t2, t1 );
    tsave += time_diff<ms>( t3, t2 );
  }
#endif

  print( "sort-disk", timestamp<ms>(), diskID, tload, tsort, tsave );
}

Sorter::Sorter( const ClusterMap & cluster, string op, string arg1 )
{
  vector<thread> diskSorters;
  for ( size_t i = 0; i < cluster.disks(); i++ ) {
    diskSorters.emplace_back( sortDisk, ref( cluster ), i, op, arg1 );
  }
  for ( auto & ds : diskSorters ) {
    ds.join();
  }
}
