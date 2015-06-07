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

Cluster::Cluster( vector<Address> nodes )
  : files_{}
  , poller_{}
{
  for ( auto & a : nodes ) {
    files_.push_back( RemoteFile{a, READ_AHEAD} );
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
  RecordNode( Record rr, RemoteFile * ff ): r{rr}, f{ff} {}

  bool operator<( const RecordNode & b ) const { return r < b.r; }
};

vector<Record> Cluster::DoRead( size_type pos, size_type size )
{
  vector<Record> recs;
  recs.reserve( size );
  mystl::priority_queue<RecordNode> heap { files_.size() };

  // seek & prefetch all remote files
  for ( auto & f : files_ ) {
    f.seek( pos );
    f.prefetch();
  }

  // load first record from each remote
  for ( auto & f : files_ ) {
    heap.push( RecordNode{ f.read(), &f } );
  }

  // merge all remote files
  for ( size_type i = 0; i < size; i++ ) {
    // grab next record
    RecordNode next { heap.top() };
    heap.pop();
    recs.push_back( next.r );

    // advance that file
    next.r = next.f->read();
    heap.push( next );
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
