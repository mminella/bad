#include "tune_knobs.hh"

#include "buffered_io.hh"
#include "exception.hh"
#include "sync_print.hh"
#include "util.hh"

#include "cluster.hh"
#include "meth1_memory.hh"
#include "priority_queue.hh"
#include "remote_file.hh"

using namespace std;
using namespace meth1;

uint64_t calc_client_buffer( size_t nodes )
{
  static_assert( sizeof( uint64_t ) >= sizeof( size_t ), "uint64_t >= size_t" );

  uint64_t memFree = memory_exists() - Knobs::MEM_RESERVE;
  memFree -= Cluster::WRITE_BUF * Cluster::WRITE_BUF_N;
  return memFree / CircularIO::BLOCK / nodes;
}

Cluster::Cluster( vector<Address> nodes, uint64_t chunkSize )
  : clients_{}
  , chunkSize_{chunkSize}
  , bufSize_{0}
{
  for ( auto & n : nodes ) {
    clients_.push_back( n );
  }

  if ( chunkSize_ == 0 and clients_.size() >= 1 ) {
    clients_[0].sendMaxChunk();
    chunkSize_ = clients_[0].recvMaxChunk();
  }
  bufSize_ = calc_client_buffer( nodes.size() );

  print( "chunk-size", chunkSize_, bufSize_ );
  print( "disks", num_of_disks() );
  print( "" );
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
  print( "\n" );

  return size;
}

Record Cluster::ReadFirst( void )
{
  for ( auto & c : clients_ ) {
    c.sendRead( 0, 1 );
  }

  char rec[Rec::SIZE];
  Record min( Rec::MAX );
  for ( auto & c : clients_ ) {
    uint64_t s = c.recvRead();
    if ( s >= 1 ) {
      c.socket().read_all( rec, Rec::SIZE );
      RecordPtr p( rec );
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
    BufferedIO bio( c.socket() );
    uint64_t totalSize = Size();
    if ( pos < totalSize ) {
      size = min( totalSize - pos, size );
      for ( uint64_t i = pos; i < pos + size; i += chunkSize_ ) {
        uint64_t n = min( chunkSize_, pos + size - i );
        c.sendRead( i, n );
        auto nrecs = c.recvRead();
        for ( uint64_t j = 0; j < nrecs; j++ ) {
          bio.read_buf_all( Rec::SIZE );
        }
      }
    }
  } else {
    // general n node case
    vector<RemoteFile *> files;
    mystl::priority_queue_min<RemoteFilePtr> pq{clients_.size()};

    // prep -- size
    for ( auto & c : clients_ ) {
      files.push_back( new RemoteFile( c, chunkSize_, bufSize_ ) );
      files.back()->sendSize();
    }

    uint64_t totalSize = 0;

    // prep -- size
    for ( auto f : files ) {
      totalSize += f->recvSize();
    }
    uint64_t end = min( totalSize, pos + size );
    print( "\n" );

    // prep -- 1st chunk
    for ( auto f : files ) {
      f->nextChunk();
    }

    // prep -- 1st record
    for ( auto f : files ) {
      f->nextRecord();
      pq.push( {f} );
    }

    // read to end records
    for ( uint64_t i = 0; i < end; i++ ) {
      RemoteFilePtr f = pq.top();
      pq.pop();
      if ( !f.eof() ) {
        f.nextRecord();
        pq.push( f );
      }
    }

    for ( auto f : files ) {
      delete f;
    }
  }
}

void Cluster::ReadAll( void )
{
  static uint64_t pass = 0;

  if ( clients_.size() == 1 ) {
    // optimize for 1 node
    auto & c = clients_.front();
    BufferedIO bio( c.socket() );
    uint64_t size = Size();
    for ( uint64_t i = 0; i < size; i += chunkSize_ ) {
      c.sendRead( i, chunkSize_ );
      auto nrecs = c.recvRead();
      auto t0 = time_now();
      for ( uint64_t j = 0; j < nrecs; j++ ) {
        bio.read_buf_all( Rec::SIZE );
      }
      print( "network", ++pass, time_diff<ms>( t0 ) );
    }
  } else {
    // general n node case
    vector<RemoteFile *> files;
    mystl::priority_queue_min<RemoteFilePtr> pq{clients_.size()};

    // prep -- size
    for ( auto & c : clients_ ) {
      files.push_back( new RemoteFile( c, chunkSize_, bufSize_ ) );
      files.back()->sendSize();
    }

    uint64_t size = 0;

    // prep -- size
    for ( auto f : files ) {
      size += f->recvSize();
    }
    print( "\n" );

    // prep -- 1st chunk
    for ( auto f : files ) {
      f->nextChunk();
    }

    // prep -- 1st record
    for ( auto f : files ) {
      f->nextRecord();
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

    for ( auto f : files ) {
      delete f;
    }
  }
}

static void writer( File out, Channel<vector<Record>> chn )
{
  tdiff_t tw = 0;
  BufferedIO bio( out );
  vector<Record> recs;
  while ( true ) {
    try {
      recs = move( chn.recv() );
    } catch ( const std::exception & e ) {
      break; // EOF
    }
    auto t0 = time_now();
    for ( auto const & r : recs ) {
      r.write( bio );
    }
    bio.flush( true );
    out.fsync();
    tw += time_diff<ms>( t0 );
  }
  print( "write", tw );
}

void Cluster::WriteAll( File out )
{
  if ( clients_.size() == 1 ) {
    // optimize for 1 node
    auto & c = clients_.front();
    BufferedIO bin( c.socket() );
    BufferedIO bout( out );
    uint64_t size = Size();
    for ( uint64_t i = 0; i < size; i += chunkSize_ ) {
      c.sendRead( i, chunkSize_ );
      auto nrecs = c.recvRead();
      for ( uint64_t j = 0; j < nrecs; j++ ) {
        // XXX: We copy between two buffers and can avoid this.
        const char * rec = bin.read_buf_all( Rec::SIZE ).first;
        bout.write_all( rec, Rec::SIZE );
      }
    }
  } else {
    // general n node case
    vector<RemoteFile *> files;
    mystl::priority_queue_min<RemoteFilePtr> pq{clients_.size()};

    Channel<vector<Record>> chn( WRITE_BUF_N - 1 );
    thread twriter( writer, move( out ), chn );

    // prep -- size
    for ( auto & c : clients_ ) {
      files.push_back( new RemoteFile( c, chunkSize_, bufSize_ ) );
      files.back()->sendSize();
    }

    uint64_t size = 0;

    // prep -- size
    for ( auto f : files ) {
      size += f->recvSize();
    }
    print( "\n" );

    // prep -- 1st chunk
    for ( auto f : files ) {
      f->nextChunk();
    }

    // prep -- 1st record
    for ( auto f : files ) {
      f->nextRecord();
      pq.push( {f} );
    }

    // read all records
    vector<Record> recs;
    recs.reserve( WRITE_BUF );
    for ( uint64_t i = 0; i < size and pq.size() > 0; i++ ) {
      RemoteFilePtr f = pq.top();
      pq.pop();
      recs.emplace_back( f.curRecord() );
      if ( !f.eof() ) {
        f.nextRecord();
        pq.push( f );
      }
      if ( recs.size() >= WRITE_BUF ) {
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

    for ( auto f : files ) {
      delete f;
    }
  }
}

void Cluster::Shutdown( void )
{
  for ( auto & c : clients_ ) {
    c.sendShutdown();
  }
}
