#include <algorithm>
#include <thread>
#include <vector>

#include "implementation.hh"

#include "client.hh"
#include "cluster.hh"
#include "priority_queue.hh"

#include "exception.hh"
#include "poller.hh"
#include "socket.hh"

using namespace std;
using namespace meth1;

Cluster::Cluster( vector<Address> nodes, size_type read_ahead )
  : files_{}
  , poller_{}
{
  for ( auto & a : nodes ) {
    files_.push_back( RemoteFile{a, read_ahead} );
  }
}

void Cluster::DoInitialize( void )
{
  for ( auto & f : files_ ) {
    f.open();
    poller_.add_action( f.RPCRunner() );
  }

  // run poller in another thread for all clients
  thread fileRPC( [this]() {
    while ( true ) {
      poller_.poll( -1 );
    }
  } );
  fileRPC.detach();
}

struct RecordNode {
  Record r;
  RemoteFile * f;
  RecordNode( Record rr, RemoteFile * ff )
    : r{rr}
    , f{ff}
  {
  }

  bool operator<( const RecordNode & b ) const { return r < b.r; }
  bool operator>( const RecordNode & b ) const { return r > b.r; }
};

vector<Record> Cluster::DoRead( size_type pos, size_type size )
{
  // PERF: Copying all records!
  // PERF: Better to copy each sorted record immediately to an output buffer
  // rather than have an intermediate vector.
  // PERF: Probably slightly better to call `DoRead` once and process whole
  // file, than use it in blocks.

  // Sorting all 
  vector<Record> recs;
  recs.reserve( size );
  mystl::priority_queue_min<RecordNode> heap{files_.size()};

  // seek & prefetch all remote files
  for ( auto & f : files_ ) {
    f.seek( pos );
    f.prefetch( size );
  }

  // load first record from each remote
  for ( auto & f : files_ ) {
    auto rs = f.peek();
    if ( rs.size() > 0 ) {
      heap.push( RecordNode{rs[0], &f} );
    }
  }

  // merge all remote files
  for ( size_type i = 0; i < size and !heap.empty(); i++ ) {
    // grab next record
    RecordNode next{heap.top()};
    recs.push_back( next.r );
    heap.pop();

    // advance that file
    if ( recs.size() < size ) {
      next.f->next();
      auto rs = next.f->peek();
      if ( rs.size() > 0 ) {
        next.r = rs[0];
        heap.push( next );
      }
    }
  }

  return recs;
}

Cluster::size_type Cluster::DoSize( void )
{
  // TODO: need multi-cast?

  // retrieve and merge results
  size_type siz{0};
  for ( auto & f : files_ ) {
    siz += f.stat();
  }

  return siz;
}
