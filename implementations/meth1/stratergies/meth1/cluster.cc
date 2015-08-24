#include <vector>

#include "buffered_io.hh"
#include "channel.hh"
#include "file.hh"
#include "util.hh"

#include "record.hh"

#include "client.hh"
#include "cluster.hh"
#include "exception.hh"
#include "priority_queue.hh"
#include "remote_file.hh"

using namespace std;
using namespace meth1;

Cluster::Cluster( vector<Address> nodes, uint64_t chunkSize )
  : clients_{}
  , chunkSize_{chunkSize}
  , bufSize_{0}
{
  auto memFree_ = memory_free() - MEM_RESERVE;
  // figure out max buffer if using all memory
  if ( chunkSize_ == 0 ) {
    chunkSize_ = memFree_ / Rec::SIZE;
  } 
  auto bufLimit = MAX_BUF_SIZE;
  bufSize_ = min( bufLimit, chunkSize_ / ( nodes.size() + WRITE_BUF_N ) );
  cout << "chunk-size, " << chunkSize_ << ", " << bufSize_ << endl;

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
    mystl::priority_queue_min<RemoteFile> pq{clients_.size()};
    uint64_t totalSize = Size();
    uint64_t end = min( totalSize, pos + size );

    // prep -- size
    for ( auto & c : clients_ ) {
      RemoteFile f( c, chunkSize_, bufSize_ );
      f.sendSize();
      files.push_back( f );
    }

    // prep -- 1st chunk
    for ( auto & f : files ) {
      f.recvSize();
      f.nextChunk( size );
    }

    // prep -- 1st record
    for ( auto & f : files ) {
      f.nextRecord( size );
      pq.push( f );
    }

    // read to end records
    for ( uint64_t i = 0; i < end; i++ ) {
      RemoteFile f = pq.top();
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
    mystl::priority_queue_min<RemoteFile> pq{clients_.size()};
    uint64_t size = Size();

    // prep -- size
    for ( auto & c : clients_ ) {
      RemoteFile f( c, chunkSize_, bufSize_ );
      f.sendSize();
      files.push_back( f );
    }

    // prep -- 1st chunk
    for ( auto & f : files ) {
      f.recvSize();
      f.nextChunk();
    }

    // prep -- 1st record
    for ( auto & f : files ) {
      f.nextRecord();
      pq.push( f );
    }

    // read all records
    for ( uint64_t i = 0; i < size; i++ ) {
      RemoteFile f = pq.top();
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
    mystl::priority_queue_min<RemoteFile> pq{clients_.size()};
    uint64_t size = Size();

    Channel<vector<Record>> chn( WRITE_BUF_N - 1 );
    thread twriter( writer, move( out ), chn );

    // prep -- size
    for ( auto & c : clients_ ) {
      RemoteFile f( c, chunkSize_, bufSize_ );
      f.sendSize();
      files.push_back( f );
    }

    // prep -- 1st chunk
    for ( auto & f : files ) {
      f.recvSize();
      f.nextChunk();
    }

    // prep -- 1st record
    for ( auto & f : files ) {
      f.nextRecord();
      pq.push( f );
    }

    // read all records
    vector<Record> recs;
    recs.reserve( bufSize_ );
    for ( uint64_t i = 0; i < size and pq.size() > 0; i++ ) {
      RemoteFile f = pq.top();
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

