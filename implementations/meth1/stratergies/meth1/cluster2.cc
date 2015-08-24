#include <thread>
#include <vector>

#include "buffered_io.hh"
#include "file.hh"
#include "poller.hh"

#include "record.hh"

#include "implementation.hh"

#include "client2.hh"
#include "cluster2.hh"
#include "priority_queue.hh"

using namespace std;
using namespace meth1;

Cluster2::Cluster2( vector<Address> nodes, uint64_t chunkSize )
  : clients_{}
  , chunkSize_{chunkSize}
{
  for ( auto & n : nodes ) {
    clients_.push_back( n );
  }
}

uint64_t Cluster2::Size( void )
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

Record Cluster2::ReadFirst( void )
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

// TODO: dynamically adjust low limit based on number of backends and available memory
static const size_t LOW_LIMIT = 1048576; // 100MB

class H
{
public:
  Client2 * c_;
  uint64_t chunkSize_;
  uint64_t size_ = 0;
  uint64_t offset_ = 0;

  bool readRPC_ = false;
  uint64_t onWire_ = 0;
  uint64_t inBuf_ = 0;
  uint8_t * buf_ = nullptr;
  uint64_t bufOffset_ = 0;

  RecordPtr head_;

public:
  H( Client2 & c, uint64_t chunkSize )
    : c_{&c}, chunkSize_{chunkSize}, head_{(const char *) nullptr}
  {};

  void sendSize( void )
  {
    c_->sendSize();
  }

  void recvSize( void )
  {
    size_ = c_->recvSize();
  }

  void nextChunk()
  {
    if ( offset_ < size_ ) {
      c_->sendRead( offset_, chunkSize_ );
      offset_ += chunkSize_;
      readRPC_ = true;
    }
  }

  bool eof( void ) const noexcept
  {
    if ( offset_ >= size_ and onWire_ == 0 and inBuf_ == 0 and !readRPC_ ) {
      return true;
    } else {
      return false;
    }
  }

  void copyWire( void )
  {
    if ( onWire_ > 0 and onWire_ <= LOW_LIMIT ) {
      if ( buf_ == nullptr ) {
        buf_ = new uint8_t[LOW_LIMIT * Rec::SIZE];
      }
      c_->sock_.read_all( (char *) buf_, onWire_ * Rec::SIZE );
      inBuf_ = onWire_;
      onWire_ = 0;
      bufOffset_ = 0;
    }
  }

  RecordPtr curRecord( void )
  {
    return head_;
  }

  void nextRecord( void )
  {
    if ( onWire_ == 0 and inBuf_ == 0 ) {
      if ( !readRPC_ ) {
        nextChunk();
      }
      onWire_ = c_->recvRead();
      readRPC_ = false;
    }

    if ( onWire_ > 0 ) {
      if ( onWire_ <= LOW_LIMIT ) {
        copyWire();
        nextChunk();
      } else {
        onWire_--;
        head_ = c_->readRecord();
        return;
      }
    }

    if ( inBuf_ > 0 ) {
      head_ = { buf_ + bufOffset_ };
      bufOffset_ += Rec::SIZE;
      inBuf_--;
      return;
    }
  }

  bool operator>( const H & b ) const
  {
    return head_ > b.head_;
  }
};

void Cluster2::Read( uint64_t pos, uint64_t size )
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

void Cluster2::ReadAll( void )
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
    vector<H> hs;
    mystl::priority_queue_min<H> pq{clients_.size()};
    uint64_t size = Size();

    // prep -- size
    for ( auto & c : clients_ ) {
      H h( c, chunkSize_ );
      h.sendSize();
      hs.push_back( h );
    }

    // prep -- 1st chunk
    for ( auto & h : hs ) {
      h.recvSize();
      h.nextChunk();
    }

    // prep -- 1st record
    for ( auto & h : hs ) {
      h.nextRecord();
      pq.push( h );
    }

    // read all records
    for ( uint64_t i = 0; i < size; i++ ) {
      H h = pq.top();
      pq.pop();
      if ( !h.eof() ) {
        h.nextRecord();
        pq.push( h );
      }
    }
  }
}

// TODO: Overlap write + read better
void Cluster2::WriteAll( File out )
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
    vector<H> hs;
    mystl::priority_queue_min<H> pq{clients_.size()};
    uint64_t size = Size();

    // prep -- size
    for ( auto & c : clients_ ) {
      H h( c, chunkSize_ );
      h.sendSize();
      hs.push_back( h );
    }

    // prep -- 1st chunk
    for ( auto & h : hs ) {
      h.recvSize();
      h.nextChunk();
    }

    // prep -- 1st record
    for ( auto & h : hs ) {
      h.nextRecord();
      pq.push( h );
    }

    // read all records
    for ( uint64_t i = 0; i < size and pq.size() > 0; i++ ) {
      H h = pq.top();
      pq.pop();
      h.curRecord().write( out );
      if ( !h.eof() ) {
        h.nextRecord();
        pq.push( h );
      }
    }
  }
}

