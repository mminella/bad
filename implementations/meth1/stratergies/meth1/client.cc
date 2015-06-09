#include <algorithm>
#include <condition_variable>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

#include "implementation.hh"

#include "client.hh"

#include "exception.hh"
#include "poller.hh"
#include "socket.hh"

using namespace std;
using namespace meth1;
using namespace PollerShortNames;

Client::Client( Address node )
  : sock_{}
  , addr_{node}
  , rpcActive_{None_}
  , rpcPos_{0}
  , rpcSize_{0}
  , cache_{}
  , lru_{}
  , size_{0}
  , mtx_{new mutex{}}
  , cv_{new condition_variable{}}
{
}

void Client::DoInitialize( void ) { sock_.connect( addr_ ); }

void Client::waitOnRPC( unique_lock<mutex> & lck )
{
  while ( rpcActive_ != None_ ) {
    cv_->wait( lck );
  }
}

void Client::sendRead( unique_lock<mutex> & lck, size_type pos, size_type siz )
{
  waitOnRPC( lck );
  rpcActive_ = Read_;
  rpcPos_ = pos;
  rpcSize_ = siz;

  int8_t rpc = 0;
  sock_.write( (char *)&rpc, 1 );
  sock_.write( (char *)&pos, sizeof( size_type ) );
  sock_.write( (char *)&siz, sizeof( size_type ) );
}

void Client::sendSize( unique_lock<mutex> & lck )
{
  waitOnRPC( lck );
  rpcActive_ = Size_;

  int8_t rpc = 1;
  sock_.write( (char *)&rpc, 1 );
}

std::vector<Record> Client::recvRead( void )
{
  // deserialize from the network
  string str = sock_.read( sizeof( size_type ) );
  size_type nrecs = *reinterpret_cast<const size_type *>( str.c_str() );

  vector<Record> recs{};
  recs.reserve( nrecs );
  for ( size_type i = 0; i < nrecs; i++ ) {
    string r = sock_.read( Record::SIZE );
    recs.push_back( Record::ParseRecord( r, 0, true ) );
  }

  if ( size_ == 0 && nrecs != rpcSize_ ) {
    // the node returns less than we requested only when at the end of the file
    size_ = rpcPos_ + nrecs;
  }

  // should an extent be evicted from cache?
  if ( lru_.size() >= MAX_CACHED_EXTENTS ) {
    size_type evict = lru_.back();
    lru_.pop_back();
    for ( auto & buf : cache_ ) {
      if ( buf.fpos == evict ) {
        // add to buffer cache, replacing evicted
        buf = {recs, rpcPos_};
      }
    }
  } else {
    // add to buffer cache
    cache_.push_back( {recs, rpcPos_} );
  }

  // maintain sort invariant
  sort( cache_.begin(), cache_.end() );

  // add to LRU
  lru_.push_front( rpcPos_ );

  return recs;
}

void Client::recvSize( void )
{
  string str = sock_.read( sizeof( size_type ) );
  size_ = *reinterpret_cast<const size_type *>( str.c_str() );
}

bool Client::fillFromCache( vector<Record> & recs, size_type & pos,
                            size_type & size )
{
  if ( size == 0 ) {
    return true;
  }

  for ( auto const & buf : cache_ ) {
    if ( buf.fpos <= pos and pos < buf.fpos + buf.records.size() ) {
      size_type start = pos - buf.fpos;
      if ( start + size <= buf.records.size() ) {
        // cache extent fully contains read
        recs.insert( recs.end(), buf.records.begin() + start,
                     buf.records.begin() + start + size );
        return true;
      } else {
        // cache extent partially contains read, other cache extents may
        // have rest of needed data.
        recs.insert( recs.end(), buf.records.begin() + start,
                     buf.records.end() );
        size_type read = buf.records.size() - start;
        pos += read;
        size -= read;
      }
    } else if ( buf.fpos > pos ) {
      // stop as soon as first gap when doing a linear scan from left we
      // could try to fill still from the cache, but we're mostly trying
      // to optimize for read-ahead, so we just do the read now.
      break;
    }
  }

  return false;
}

vector<Record> Client::DoRead( size_type pos, size_type size )
{
  unique_lock<mutex> lck{*mtx_};
  vector<Record> recs{};

  // Don't read past end of file
  if ( size_ != 0 && pos + size > size_ ) {
    size = size_ - pos;
  }

  recs.reserve( size );

  // fill from cache if possible
  if ( fillFromCache( recs, pos, size ) ) {
    return recs;
  }

  // perform read for remaining data missing from cache (but make sure we
  // don't already have an RPC outstanding for same data)
  if ( rpcActive_ != Read_ or rpcPos_ != pos ) {
    sendRead( lck, pos, size );
  }

  waitOnRPC( lck );

  // validate RPC was correct
  if ( lru_.front() != pos ) {
    throw runtime_error( "new cached extent isn't what was expected" );
  }

  // Don't read past end of file (again, since may have reached eof with last read)
  if ( size_ != 0 && pos + size > size_ ) {
    size = size_ - pos;
  }
  
  // we let it fail this time, which it should only do if we are trying to read
  // past the end of the file.
  fillFromCache( recs, pos, size );

  return recs;
}

Client::size_type Client::DoSize( void )
{
  unique_lock<mutex> lck{*mtx_};
  if ( size_ == 0 ) {
    if ( rpcActive_ != Size_ ) {
      sendSize( lck );
    }
    waitOnRPC( lck );
  }
  return size_;
}

Action Client::RPCRunner( void )
{
  return Action{sock_, Direction::In, [this]() {
    unique_lock<mutex> lck{*mtx_};

    switch ( rpcActive_ ) {
    case None_:
      throw runtime_error( "socket ready to read but no rpc" );
      break;
    case Read_:
      recvRead();
      break;
    case Size_:
      recvSize();
      break;
    }

    rpcActive_ = None_;
    cv_->notify_one();

    return ResultType::Continue;
  }};
}
