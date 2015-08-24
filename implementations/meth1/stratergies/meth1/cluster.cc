#include <thread>
#include <vector>

#include "buffered_io.hh"
#include "file.hh"
#include "poller.hh"

#include "record.hh"

#include "implementation.hh"

#include "client.hh"
#include "cluster.hh"
#include "priority_queue.hh"
#include "remote_file.hh"

using namespace std;
using namespace meth1;

Cluster::Cluster( vector<Address> nodes, uint64_t chunkSize )
  : clients_{}
  , chunkSize_{chunkSize}
{
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
    // TODO: multi-node Read
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
      RemoteFile f( c, chunkSize_ );
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

// TODO: Overlap write + read better
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

    // prep -- size
    for ( auto & c : clients_ ) {
      RemoteFile f( c, chunkSize_ );
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
    for ( uint64_t i = 0; i < size and pq.size() > 0; i++ ) {
      RemoteFile f = pq.top();
      pq.pop();
      f.curRecord().write( out );
      if ( !f.eof() ) {
        f.nextRecord();
        pq.push( f );
      }
    }
  }
}

