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

void Cluster::ReadChunk( uint64_t size )
{
  if ( files_.size() == 1 ) {
    // optimize for 1 node
    Size();
    auto & c = files_[0].client();
    c.Read( 0, size );

  } else {
    // general n node case
    Size();

    // seek & prefetch all remote files
    for ( auto & f : files_ ) {
      f.client().prepareRead( 0, size );
    }

    // load first record from each remote
    for ( auto & f : files_ ) {
      f.client().Read( 0, size );
    }
  }
}

void Cluster::ReadAll( void )
{
  if ( files_.size() == 1 ) {
    // optimize for 1 node
    Size();
    auto & f = files_[0];
    f.seek( 0 );
    while ( !f.eof() ) {
      auto recs = f.read_chunk();
      if ( recs.size() == 0 ) {
        break;
      }
    }

  } else {
    // general n node case
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
}

static void writer( File out, Channel<std::vector<Record>> chn )
{
  static int pass = 0;
  BufferedIO bio( out );
  vector<Record> recs;
  while ( true ) {
    try {
      recs = move( chn.recv() );
    } catch ( const std::exception & e ) {
      break;
    }
    auto t0 = time_now();
    for ( auto const & r : recs ) {
      r.write( bio );
    }
    bio.flush( true );
    out.fsync();
    auto twrite = time_diff<ms>( t0 );
    cout << "write, " << pass++ << ", " << twrite << endl;
  }
}

void Cluster::WriteAll( File out )
{
  if ( files_.size() == 1 ) {
    // optimize for 1 node
    Size();
    auto & f = files_[0];
    f.seek( 0 );

    Channel<vector<Record>> chn( 0 );
    thread twriter( writer, move( out ), chn );

    while ( !f.eof() ) {
      auto recs = f.read_chunk();
      if ( recs.size() == 0 ) {
        break;
      }
      chn.send( move( recs ) );
    }
    chn.send( {} );

    // wait for write to finish
    chn.close();
    twriter.join();

  } else {
    // general n node case
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
}
