#include <iostream>

#include "sync_print.hh"
#include "timestamp.hh"

#include "recv.hh"

using namespace std;
using namespace PollerShortNames;

// Allocation helper
static uint8_t * newBlock( void )
{
  return (uint8_t *) malloc( Receiver::DISK_BLOCK_SIZE );
}

// De-allocation helper
static void freeBlock( uint8_t * buf )
{
  free( buf );
}

NetIn::NetIn( ClusterMap & cluster, vector<DiskWriter> & disks, TCPSocket sock )
  : cluster_{cluster}
  , disks_{disks}
  , bucketsLive_{cluster.myBuckets().size()}
  , sock_{move( sock )}
  , wireState_{IDLE}
  , headerOnWire_{0}
  , bucketOnWire_{0}
  , bucketLocalID_{0}
  , bodyOnWire_{0}
  , cantMove_{false}
{
}

NetIn::NetIn( NetIn && other )
  : cluster_{other.cluster_}
  , disks_{other.disks_}
  , bucketsLive_{other.bucketsLive_}
  , sock_{move( other.sock_ )}
  , wireState_{other.wireState_}
  , headerOnWire_{other.headerOnWire_}
  , bucketOnWire_{other.bucketOnWire_}
  , bucketLocalID_{other.bucketLocalID_}
  , bodyOnWire_{other.bodyOnWire_}
  , cantMove_{other.cantMove_}
{
  if ( other.cantMove_  ) {
    // FIXME: Hack to allow us to easily take a reference to a NetIn for
    // Polling, but at least catch when this breaks.
    throw runtime_error( "Moved unmoveable NetIn" );
  }

  other.cantMove_ = true; // now it's invalid...
  other.bucketsLive_ = 0;
  other.wireState_ = DONE;
  other.headerOnWire_ = 0;
  other.bodyOnWire_ = 0;
}

bool NetIn::read( std::vector<block_t> & buckets )
{
  size_t n, rmax, offset;
  const uint8_t * rpcData = header_; // work-around strict-aliasing rules
  block_t * block;

  while ( true ) {
    switch ( wireState_ )
    {
    case IDLE:
      wireState_ = HEADER;
      headerOnWire_ = HDRSIZE;

    case HEADER:
      if ( headerOnWire_ == 0 ) {
        throw runtime_error( "No header to read from wire" );
      }
      offset = HDRSIZE - headerOnWire_;
      n = sock_.read( (char *) header_ + offset, headerOnWire_ );
      headerOnWire_ -= n;
      if ( n == 0 ) {
        return true;
      } else if ( headerOnWire_ == 0 ) {
        wireState_ = PARSE;
      } else {
        continue;
      }

    case PARSE:
      bucketOnWire_ = *reinterpret_cast<const uint16_t *>( rpcData );
      bodyOnWire_ = *reinterpret_cast<const uint64_t *>( rpcData + 2 );
      bucketLocalID_ = cluster_.bucket_local_id( bucketOnWire_  );
      if ( bodyOnWire_ == 0 ) { // EOF -- bucket
        bucketsLive_--;
        if ( bucketsLive_ == 0 ) {
          wireState_ = DONE;
          return false; // EOF -- socket
        } else {
          wireState_ = IDLE;
          continue;
        }
      }
      wireState_ = BODY;

    case BODY:
      if ( bodyOnWire_ == 0 ) {
        throw runtime_error( "No header to read from wire" );
      }
      block = &buckets[bucketLocalID_];
      if ( block->bucket != bucketOnWire_ ) {
        throw runtime_error( "Wrong bucket selected" );
      }
      rmax = min( Receiver::DISK_BLOCK_SIZE - block->len, bodyOnWire_ );
      n = sock_.read( (char *) block->buf + block->len, rmax );
      if ( n == 0 ) {
        return true;
      }
      block->len += n;
      bodyOnWire_ -= n;

      if ( block->len == Receiver::DISK_BLOCK_SIZE or bodyOnWire_ == 0 ) {
        DiskWriter & dw = disks_[cluster_.bucket_disk( bucketOnWire_ )];
        dw.send( *block );
        block->buf = newBlock();
        block->len = 0;
        if ( bodyOnWire_ == 0 ) {
          wireState_ = IDLE;
        }
      }
      break;

    case DONE:
      throw runtime_error( "Called read for a finished NetIn" );

    default:
      print( "p1", "bad-wire", wireState_, bucketOnWire_,
        headerOnWire_, bodyOnWire_ );
      throw runtime_error( "Wire in invalid state" );
    }
  }
}

Receiver::Receiver( ClusterMap & cluster, Address address )
  : cluster_{cluster}
  , poll_{}
  , sock_{IPV4}
  , netins_{}
  , buckets_{cluster.myBuckets().size()}
  , backendsLive_{cluster_.nodes()}
  , disks_{}
{
  sock_.set_reuseaddr();
  sock_.set_nodelay();
  sock_.set_send_buffer( Knobs4::NET_SND_BUF );
  sock_.set_recv_buffer( Knobs4::NET_RCV_BUF );
  sock_.bind( address );
  sock_.listen();

  print( "p0", "listen", sock_.local_address().to_string() );

  for ( size_t i = 0; i < buckets_.size(); i++ ) {
    buckets_[i] = {newBlock(), 0, cluster_.myBuckets()[i]};
  }

  for ( size_t i = 0; i < cluster_.disk_paths().size(); i++) {
    disks_.emplace_back( cluster_, i, cluster_.disk_paths()[i] );
    disks_.back().start();
  }
}

Receiver::~Receiver( void )
{
  for ( auto & b : buckets_ ) {
    freeBlock( b.buf );
  }
}

void Receiver::waitForConnections( void )
{
  for ( size_t i = 0; i < cluster_.nodes(); i++ ) {
    TCPSocket s = sock_.accept();
    s.set_nodelay();
    s.set_send_buffer( Knobs4::NET_SND_BUF );
    s.set_recv_buffer( Knobs4::NET_RCV_BUF );
    if ( NET_NON_BLOCKING ) {
      s.set_non_blocking();
    }
    netins_.emplace_back( cluster_, disks_, move( s ) );
    print( "p0", "new-connection",
      netins_.back().socket().peer_address().to_string() );
  }

  // Setup polling on all sockets
  for ( auto & n : netins_ ) {
    n.disableMove();
    poll_.add_action({ n.socket(), Direction::In, [&n, this]() {
      bool alive = n.read( buckets_ );
      if ( not alive ) {
        backendsLive_--;
        if ( backendsLive_ == 0 ) {
          return ResultType::Exit;
        } else {
          return ResultType::Cancel;
        }
      } else {
        return ResultType::Continue;
      }
    }});
  }
}

void Receiver::receiveLoop( void )
{
  auto t0 = time_now();
  print( "p1", "recv-start", timestamp<ms>() );
  poll_.loop();
  print( "p1", "recv-end", timestamp<ms>(), time_diff<ms>( t0 ) );
}

void Receiver::waitFinished( void )
{
  for ( auto & d : disks_ ) {
    d.waitDrained( true );
  }
}
