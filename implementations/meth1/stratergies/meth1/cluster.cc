#include "tune_knobs.hh"

#include "buffered_io.hh"
#include "exception.hh"
#include "util.hh"

#include "cluster.hh"
#include "meth1_memory.hh"
#include "priority_queue.hh"
#include "remote_file.hh"

using namespace std;
using namespace meth1;

uint64_t calc_client_buffer( size_t nodes )
{
  uint64_t memFree = memory_free() - Knobs::MEM_RESERVE;
  // We have a max buffer as there is a trade-off between more copying with
  // larger buffers vs. more change of overlapping reads at backend nodes.
  uint64_t bufLimit = Cluster::MAX_BUF_SIZE / Rec::SIZE;
  uint64_t perClient = memFree / Rec::SIZE / ( nodes + Cluster::WRITE_BUF_N );
  return min( bufLimit, perClient );
}

Cluster::Cluster( vector<Address> nodes, uint64_t chunkSize )
  : clients_{}
  , chunkSize_{chunkSize}
  , bufSize_{0}
{
  if ( chunkSize_ == 0 ) {
    chunkSize_ = calc_record_space();
  } 
  bufSize_ = calc_client_buffer( nodes.size() );

  cout << "chunk-size, " << chunkSize_ << ", " << bufSize_ << endl;
  cout << "disks, " << num_of_disks() << endl;

  for ( auto & n : nodes ) {
    clients_.push_back( n );
  }
}

uint64_t Cluster::Size( void )
{
  for ( auto & c : clients_ ) {
    c.sendSize();
  }
  
  uint64_t size = 0;
  for ( auto & c : clients_ ) {
    size += c.recvSize();
  }

  return size;
}

Record Cluster::ReadFirst( void )
{
  for ( auto & c : clients_ ) {
    c.sendRead( 0, 1 );
  }

  Record min( Rec::MAX );
  for ( auto & c : clients_ ) {
    uint64_t s = c.recvRead();
    if ( s >= 1 ) {
      RecordPtr p = c.readRecord();
      if ( p < min ) {
        min.copy( p );
      }
    }
  }

  return min;
}

void Cluster::Read( uint64_t pos, uint64_t size )
{
  if ( clients_.size() == 1 ) {
    // optimize for 1 node
    auto & c = clients_.front();
    uint64_t totalSize = Size();
    if ( pos < totalSize ) {
      size = min( totalSize - pos, size );
      for ( uint64_t i = pos; i < pos + size; i += chunkSize_ ) {
        uint64_t n = min( chunkSize_, pos + size - i );
        c.sendRead( i, n );
        auto nrecs = c.recvRead();
        for ( uint64_t j = 0; j < nrecs; j++ ) {
          c.readRecord();
        }
      }
    }
  } else {
    // general n node case
    vector<RemoteFile> files;
    mystl::priority_queue_min<RemoteFilePtr> pq{clients_.size()};
    uint64_t totalSize = Size();
    uint64_t end = min( totalSize, pos + size );

    // prep -- size
    for ( auto & c : clients_ ) {
      files.emplace_back( c, chunkSize_, bufSize_ );
      files.back().sendSize();
    }

    // prep -- 1st chunk
    for ( auto & f : files ) {
      f.recvSize();
      f.nextChunk( size );
    }

    // prep -- 1st record
    for ( auto & f : files ) {
      f.nextRecord( size );
      pq.push( {f} );
    }

    // read to end records
    for ( uint64_t i = 0; i < end; i++ ) {
      RemoteFilePtr f = pq.top();
      pq.pop();
      if ( !f.eof() ) {
        f.nextRecord( size - i );
        pq.push( f );
      }
    }
  }
}

void Cluster::ReadAll( void )
{
  if ( clients_.size() == 1 ) {
    // optimize for 1 node
    auto & c = clients_.front();
    uint64_t size = Size();
    for ( uint64_t i = 0; i < size; i += chunkSize_ ) {
      c.sendRead( i, chunkSize_ );
      auto nrecs = c.recvRead();
      for ( uint64_t j = 0; j < nrecs; j++ ) {
        c.readRecord();
      }
    }
  } else {
    // general n node case
    vector<RemoteFile> files;
    mystl::priority_queue_min<RemoteFilePtr> pq{clients_.size()};
    uint64_t size = Size();

    // prep -- size
    for ( auto & c : clients_ ) {
      files.emplace_back( c, chunkSize_, bufSize_ );
      files.back().sendSize();
    }

    // prep -- 1st chunk
    for ( auto & f : files ) {
      f.recvSize();
      f.nextChunk();
    }

    // prep -- 1st record
    for ( auto & f : files ) {
      f.nextRecord();
      pq.push( {f} );
    }

    // read all records
    for ( uint64_t i = 0; i < size; i++ ) {
      RemoteFilePtr f = pq.top();
      pq.pop();
      if ( !f.eof() ) {
        f.nextRecord();
        pq.push( f );
      }
    }
  }
}

static void writer( File out, Channel<vector<Record>> chn )
{
  static int pass = 0;
  BufferedIO bio( out );
  vector<Record> recs;
  while ( true ) {
    try {
      cout << "wait write!" << endl;
      recs = move( chn.recv() );
      cout << "got write!" << endl;
    } catch ( const std::exception & e ) {
      print_exception( e );
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
  if ( clients_.size() == 1 ) {
    // optimize for 1 node
    auto & c = clients_.front();
    uint64_t size = Size();
    for ( uint64_t i = 0; i < size; i += chunkSize_ ) {
      c.sendRead( i, chunkSize_ );
      auto nrecs = c.recvRead();
      for ( uint64_t j = 0; j < nrecs; j++ ) {
        RecordPtr rec = c.readRecord();
        rec.write( out );
      }
    }
  } else {
    // general n node case
    vector<RemoteFile> files;
    mystl::priority_queue_min<RemoteFilePtr> pq{clients_.size()};
    uint64_t size = Size();

    Channel<vector<Record>> chn( WRITE_BUF_N - 1 );
    thread twriter( writer, move( out ), chn );

    // prep -- size
    for ( auto & c : clients_ ) {
      files.emplace_back( c, chunkSize_, bufSize_ );
      files.back().sendSize();
    }

    // prep -- 1st chunk
    for ( auto & f : files ) {
      f.recvSize();
      f.nextChunk();
    }

    // prep -- 1st record
    for ( auto & f : files ) {
      f.nextRecord();
      pq.push( {f} );
    }

    // read all records
    vector<Record> recs;
    recs.reserve( bufSize_ );
    for ( uint64_t i = 0; i < size and pq.size() > 0; i++ ) {
      RemoteFilePtr f = pq.top();
      pq.pop();
      recs.emplace_back( f.curRecord() );
      if ( !f.eof() ) {
        f.nextRecord();
        pq.push( f );
      }
      if ( recs.size() >= bufSize_ ) {
        chn.send( move( recs ) );
        recs = vector<Record>();
      }
    }

    // wait for write to finish
    if ( recs.size() > 0 ) {
      chn.send( move( recs ) );
    }
    chn.waitEmpty();
    chn.close();
    twriter.join();
  }
}

void Cluster::Shutdown( void )
{
  for ( auto & c : clients_ ) {
    c.sendShutdown();
  }
}
