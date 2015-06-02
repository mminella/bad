#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>

#include "implementation.hh"

#include "client.hh"
#include "cluster.hh"

#include "socket.hh"

using namespace std;
using namespace meth1;

Cluster::Cluster( vector<Address> nodes )
  : nodes_{}
{
  for ( auto & a : nodes ) {
    nodes_.push_back( Client{a} );
  }
}

void Cluster::DoInitialize( void )
{
  for ( auto & n : nodes_ ) {
    n.Initialize();
  }
}

vector<Record> Cluster::DoRead( size_type pos, size_type size )
{
  vector<Record> recs;
  recs.reserve( nodes_.size() * size );

  // multi-cast RPC
  for ( auto & n : nodes_ ) {
    n.sendRead( pos, size );
  }

  // retrieve and merge results
  for ( auto & n : nodes_ ) {
    auto rs = n.recvRead();
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
  for ( auto & n : nodes_ ) {
    n.sendSize();
  }

  // retrieve and merge results
  size_type siz{0};
  for ( auto & n : nodes_ ) {
    siz += n.recvSize();
  }

  return siz;
}
