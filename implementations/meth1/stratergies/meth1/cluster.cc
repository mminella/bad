#include <thread>
#include <vector>

#include "buffered_io.hh"
#include "file.hh"
#include "poller.hh"

#include "implementation.hh"

#include "client.hh"
#include "cluster.hh"
#include "priority_queue.hh"

using namespace std;
using namespace meth1;

Cluster::Cluster( vector<Address> nodes, uint64_t readahead )
  : files_{}
  , poller_{}
{
  for ( auto & n : nodes ) {
    files_.push_back( RemoteFile{n, readahead} );
  }

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
  // TODO: make a pointer type?
  Record r;
  RemoteFile * f;

  RecordNode( Record rr, RemoteFile * ff ) : r{rr}, f{ff} {}
  RecordNode( const RecordNode & rn ) : r{rn.r}, f{rn.f} {}

  RecordNode & operator=( const RecordNode & rn )
  {
    if ( this != &rn ) {
      r = rn.r;
      f = rn.f;
    }
    return *this;
  }

  bool operator<( const RecordNode & b ) const { return r < b.r; }
  bool operator>( const RecordNode & b ) const { return r > b.r; }
};

uint64_t Cluster::Size( void )
{
  uint64_t siz = 0;
  // send size RPC to each
  for ( auto & f : files_ ) {
    f.client().prepareSize();
  }
  for ( auto & f : files_ ) {
    siz += f.size();
  }
  return siz;
}

Record Cluster::ReadFirst( void )
{
  mystl::priority_queue_min<RecordNode> pq{files_.size()};
  for ( auto & f : files_ ) {
    f.seek( 0 );
    f.client().prepareRead( 0, 1 );
  }
  for ( auto & f : files_ ) {
    auto v = f.client().Read( 0, 1 );
    if ( v.size() > 0 ) {
      pq.emplace( move( v[0] ), &f );
    }
  }
  return pq.top().r;
}

void Cluster::ReadAll( void )
{
  mystl::priority_queue_min<RecordNode> pq{files_.size()};

  uint64_t size = Size();

  // seek & prefetch all remote files
  for ( auto & f : files_ ) {
    f.seek( 0 );
    f.prefetch();
  }

  // load first record from each remote
  for ( auto & f : files_ ) {
    auto r = f.read();
    if ( r != nullptr ) {
      pq.emplace( move( *r ), &f );
    }
  }

  // merge all remote files
  for ( uint64_t i = 0; i < size and !pq.empty(); i++ ) {
    // grab next record
    RemoteFile * f = pq.top().f;
    pq.pop();

    // advance that file
    auto r = f->read();
    if ( r != nullptr ) {
      pq.emplace( move( *r ), f );
    }
  }
}

static void writer( File out, Channel<std::vector<Record>> chn )
{
  BufferedIO bio( out );
  vector<Record> recs;
  while ( true ) {
    try {
      recs = move( chn.recv() );
    } catch ( const std::exception & e ) {
      return;
    }
    for ( auto const & r : recs ) {
      r.write( bio );
    }
    bio.flush( true );
    out.fsync();
  }
}

void Cluster::WriteAll( File out )
{
  mystl::priority_queue_min<RecordNode> pq{files_.size()};

  uint64_t size = Size();

  // seek & prefetch all remote files
  for ( auto & f : files_ ) {
    f.seek( 0 );
    f.prefetch();
  }

  // load first record from each remote
  for ( auto & f : files_ ) {
    auto r = f.read();
    if ( r != nullptr ) {
      pq.emplace( move( *r ), &f );
    }
  }

  Channel<vector<Record>> chn( 0 );
  thread twriter( writer, move( out ), chn );

  // TWEAK: chunk size
  const size_t CHUNK_SIZE = 104858; // 10MB
  vector<Record> recs;
  recs.reserve( CHUNK_SIZE );

  // merge all remote files
  for ( uint64_t i = 0; i < size and !pq.empty(); i++ ) {
    // grab next record
    recs.emplace_back( move( pq.top().r ) );
    RemoteFile * f = pq.top().f;
    pq.pop();

    // advance that file
    auto r = f->read();
    if ( r != nullptr ) {
      pq.emplace( move( *r ), f );
    }

    // send to writer
    if ( recs.size() == CHUNK_SIZE ) {
      chn.send( move( recs ) );
      recs = vector<Record>();
      recs.reserve( CHUNK_SIZE );
    }
  }
  chn.send( move( recs ) );
  chn.send( {} );

  // wait for write to finish
  chn.close();
  twriter.join();
}
