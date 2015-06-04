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
  : clients_{}
  , poller_{}
{
  for ( auto & a : nodes ) {
    clients_.push_back( Client{a} );
  }
}

void Cluster::DoInitialize( void )
{
  for ( auto & c : clients_ ) {
    c.Initialize();
    poller_.add_action( c.RPCRunner() );
  }

  // run poller in another thread for all clients
  thread clientRPC( [this]() {
    while ( true ) { poller_.poll( -1 ); }
  } );
  clientRPC.detach();
}

// struct RecordNode {
//   Record r;
//   Client * c;
//   RecordNode( Record rr, Client * cc ): r{rr}, c{cc} {}
//
//   bool operator<( const RecordNode & b ) const { return r < b.r; }
// };
//
// vector<Record> Cluster::DoRead2( size_type pos, size_type size )
// {
//   vector<Record> recs;
//   recs.reserve( size );
//   mystl::priority_queue<RecordNode> heap { clients_.size() };
//   
//   for ( auto & c : clients_ ) {
//     c.prepRead( pos, size );
//   }
//
//   for ( auto & c : clients_ ) {
//     heap.push( RecordNode{ c.top(), &c } );
//   }
//
//   for ( size_type i = 0; i < size; i++ ) {
//     // grab next record
//     RecordNode next { heap.top() };
//     heap.pop();
//     recs.push_back( next.r );
//
//     // advance that client
//     next.r = next.c->next();
//     heap.push( next );
//   }
// }

vector<Record> Cluster::DoRead( size_type pos, size_type size )
{
  vector<Record> recs;
  recs.reserve( size );

  // multi-cast RPC
  for ( auto & c : clients_ ) {
    c.prepareRead( pos, size );
  }

  // retrieve and merge results
  for ( auto & c : clients_ ) {
    auto rs = c.Read( pos, size );
    auto pend = recs.end();

    recs.insert( pend, rs.begin(), rs.end() );

    // we merge and trim as we go, but may want to cache extra results for
    // future operations.
    inplace_merge( recs.begin(), pend, recs.end() );
    recs.resize( size, Record{Record::MAX} );
  }

  return recs;
}

Cluster::size_type Cluster::DoSize( void )
{
  // multi-cast RPC
  for ( auto & c : clients_ ) {
    c.prepareSize();
  }

  // retrieve and merge results
  size_type siz{0};
  for ( auto & c : clients_ ) {
    siz += c.Size();
  }

  return siz;
}
