#include "sync_print.hh"

#include "meth4_knobs.hh"
#include "send.hh"

using namespace std;

// Allocation helper
static uint8_t * newBlock( void )
{
  return (uint8_t *) malloc( NetOut::NET_BLOCK_SIZE );
}

// De-allocation helper
static void freeBlock( uint8_t * buf )
{
  free( buf );
}

// NOTE: We don't bother to remove our own node from the list of sockets to
// listen on and send to. Instead, we let the data get transferred (locally)
// over the network stack as this is not a limiting factor and avoiding this
// would complicate the code a lot by introducing non-uniformity.

NetOut::NetOut( ClusterMap & cluster )
  : sockets_{}
  , cluster_{cluster}
  , queue_{NET_QUEUE_LENGTH}
  , netsend_{}
{
  // PERF: May need multiple threads here to saturate network
  for ( const auto & c : cluster_.addresses() ) {
    TCPSocket sock{(IPVersion) c.domain()};
    sock.set_nodelay();
    sock.set_send_buffer( Knobs4::NET_SND_BUF );
    sock.set_recv_buffer( Knobs4::NET_RCV_BUF );
    print( "p0", "connect", c.to_string() );
    sock.connect( c );
    sockets_.push_back( move( sock ) );
  }

  netsend_ = thread( &NetOut::sendLoop, this );
}

NetOut::~NetOut( void )
{
  queue_.waitEmpty();
  queue_.close();
  if ( netsend_.joinable() ) { netsend_.join(); }
}

void sendRPCHeader( TCPSocket & sock, uint16_t bkt, size_t len )
{
  constexpr size_t HDRSIZE = sizeof( uint16_t ) + sizeof( uint64_t );
  char header[HDRSIZE];
  char * rpcData = header; // avoids breaking strict aliasing rules
  *reinterpret_cast<uint16_t *>( rpcData ) = bkt;
  *reinterpret_cast<uint64_t *>( rpcData + 2 ) = len;
  sock.write_all( header, HDRSIZE );
}

void sendRPCBody( TCPSocket & sock, uint8_t * buf, size_t len )
{
  sock.write_all( (char *) buf, len );
}

void NetOut::sendLoop( void )
{
  print( "p1", "netout-start", timestamp<ms>() );
  auto t0 = time_now();
  tdiff_t tnet = 0;

  size_t activeBuckets = cluster_.buckets() * cluster_.disks();
  try {
    while ( activeBuckets > 0 ) {
      block_t block = queue_.recv();
      size_t nodeID = cluster_.bucket_node( block.bucket );
      TCPSocket & sock = sockets_[nodeID];

      // PERF: Overhead of taking this many timestamps?
      auto t1 = time_now();
      if ( block.buf == nullptr ) {
        activeBuckets--;
        sendRPCHeader( sock, block.bucket, 0 );
        tnet += time_diff<ns>( t1 );
      } else {
        // PERF: hopefully we won't block so much here to a individual node as
        // to hurt overall network performance.
        sendRPCHeader( sock, block.bucket, block.len );
        sendRPCBody( sock, block.buf, block.len );
        tnet += time_diff<ns>( t1 );
        freeBlock( block.buf );
      }
    }
  } catch ( const Channel<block_t>::closed_error & e ) {
    // EOF
  }

  tnet /= 1000;
  print( "p1", "netout-done", timestamp<ms>(), time_diff<ms>( t0 ), tnet );
}

void NetOut::send( block_t block )
{
  queue_.send( block );
}

Sender::Sender( File & file, ClusterMap & cluster, NetOut & net  )
  : rio_{file, DISK_QUEUE_LENGTH}
  , cluster_{cluster}
  , net_{net}
  , buckets_{cluster.buckets()}
  , sorter_{}
{
  for ( uint16_t i = 0; i < buckets_.size(); i++ ) {
    buckets_[i] = {newBlock(), 0, i};
  }
}

Sender::~Sender( void )
{
  waitFinished();
  print( "p1", "send-end", timestamp<ms>(), time_diff<ms>( start_ ) );
}

void Sender::_start( void )
{
  auto t0 = time_now();
  rio_.rewind();

  while ( true ) {
    // get next record from disk
    RecordPtr rec = rio_.next_record();
    if ( rec.isNull() ) {
      break;
    }

    // place record into bucket
    size_t bkt = cluster_.bucket( rec.key() );
    block_t & bucket = buckets_[bkt];
    memcpy( bucket.buf + bucket.len, rec.data(), Rec::SIZE );
    bucket.len += Rec::SIZE;

    // send full bucket
    if ( bucket.len == NetOut::NET_BLOCK_SIZE ) {
      net_.send( bucket );
      bucket.buf = newBlock();
      bucket.len = 0;
    }
  }

  // drain all buckets
  for ( auto & bkt : buckets_ ) {
    net_.send( bkt );
    bkt.buf = nullptr;
    bkt.len = 0;
    net_.send( bkt ); // EOF
  }

  print( "p1", "disk-done", timestamp<ms>(), time_diff<ms>( t0 ) );
}

void Sender::start( void )
{
  print( "p1", "send-start", timestamp<ms>() );
  start_ = time_now();
  sorter_ = thread( &Sender::_start, this );
}

void Sender::waitFinished( void )
{
  if ( sorter_.joinable() ) { sorter_.join(); }
}

// Sanity check: just count how many records go into each bucket, don't send
// over network.
void Sender::countBucketDistribution( void )
{
  rio_.rewind();
  vector<size_t> counts( cluster_.buckets() );
  size_t recs = 0;

  for ( size_t i = 0;; i++ ) {
    RecordPtr rec = rio_.next_record();
    if ( rec.isNull() ) {
      break;
    }
    recs++;
    size_t b = cluster_.bucket( rec.key() );
    counts[b]++;
  }

  print( "p0", "records", recs );
  for ( size_t i = 0; i < counts.size(); i++ ) {
    print( "p0", "bucket", i, counts[i] );
  }
}
