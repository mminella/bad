#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <tuple>
#include <vector>

#include "buffered_io.hh"
#include "exception.hh"
#include "poller.hh"
#include "socket.hh"
#include "timestamp.hh"

#include "implementation.hh"

#include "client.hh"

using namespace std;
using namespace meth1;
using namespace PollerShortNames;

Client::Client( Address node )
  : sock_{{(IPVersion)(node.domain())}}
  , addr_{node}
  , rpcActive_{None_}
  , rpcPos_{0}
  , rpcSize_{0}
  , rpcStart_{}
  , cache_{}
  , size_{0}
  , mtx_{new mutex{}}
  , cv_{new condition_variable{}}
{
}

void Client::DoInitialize( void )
{
  sock_.io().connect( addr_ );
}

void Client::waitOnRPC( unique_lock<mutex> & lck )
{
  while ( rpcActive_ != None_ ) {
    cv_->wait( lck );
  }
}

void Client::sendRead( unique_lock<mutex> & lck, uint64_t pos, uint64_t siz )
{
  if ( rpcActive_ == Read_ and rpcPos_ == pos ) {
    return;
  }

  waitOnRPC( lck );
  rpcActive_ = Read_;
  rpcPos_ = pos;
  rpcSize_ = siz;
  rpcStart_ = time_now();

  char data[1 + 2 * sizeof( uint64_t )];
  data[0] = 0;
  *reinterpret_cast<uint64_t *>( data + 1 ) = pos;
  *reinterpret_cast<uint64_t *>( data + 1 + sizeof( uint64_t ) ) = siz;
  sock_.io().write_all( data, sizeof( data ) );
}

void Client::sendSize( unique_lock<mutex> & lck )
{
  waitOnRPC( lck );
  rpcActive_ = Size_;
  rpcStart_ = time_now();

  int8_t rpc = 1;
  sock_.io().write_all( (char *)&rpc, 1 );
}

void Client::recvRead( void )
{
  auto split = time_now();
  cout << "[Read RPC]: " << time_diff<ms>( split, rpcStart_ ) << "ms" << endl;

  auto r = sock_.read_buf_all( sizeof( uint64_t ) ).first;
  uint64_t nrecs = *reinterpret_cast<const uint64_t *>( r );

  if ( nrecs > 0 ) {
    vector<Record> recs{};
    recs.reserve( nrecs );
    for ( uint64_t i = 0; i < nrecs; i++ ) {
      r = sock_.read_buf_all( Record::SIZE ).first;
      recs.emplace_back( r );
    }

    cache_.emplace_front( move( recs ), rpcPos_ );
    if ( cache_.size() > MAX_CACHED_EXTENTS ) {
      cache_.pop_back();
    }
  }
  cout << "[Deserialize]: " << time_diff<ms>( split ) << "ms" << endl;
}

void Client::recvSize( void )
{
  auto str = sock_.read_buf_all( sizeof( uint64_t ) ).first;
  size_ = *reinterpret_cast<const uint64_t *>( str );
  cout << "[Size RPC]: " << time_diff<ms>( rpcStart_ ) << "ms" << endl;
}

vector<Record> Client::DoRead( uint64_t pos, uint64_t size )
{
  unique_lock<mutex> lck{*mtx_};

  // Don't read past end of file
  if ( size_ != 0 ) {
    if ( pos >= size_ ) {
      return {};
    } else if ( pos + size > size_ ) {
      size = size_ - pos;
    }
  }

  // fill from cache if possible
  for ( auto it = cache_.begin(); it != cache_.end(); ++it ) {
    if ( it->fpos == pos && it->records.size() == size ) {
      auto r = move( it->records );
      cache_.erase( it );
      return r;
    }
  }

  // send read RPC if haven't already
  if ( rpcActive_ != Read_ or rpcPos_ != pos ) {
    sendRead( lck, pos, size );
  }
  waitOnRPC( lck );

  // Don't read past end of file (again, since may have reached eof with last
  // read)
  if ( size_ != 0 ) {
    if ( pos >= size_ ) {
      return {};
    } else if ( pos + size > size_ ) {
      size = size_ - pos;
    }
  }

  // search cache again
  for ( auto it = cache_.begin(); it != cache_.end(); ++it ) {
    if ( it->fpos == pos && it->records.size() == size ) {
      auto r = move( it->records );
      cache_.erase( it );
      return r;
    }
  }

  throw runtime_error( "couldn't find RPC result" );
}

uint64_t Client::DoSize( void )
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
  return Action{sock_.io(), Direction::In, [this]() {
    unique_lock<mutex> lck{*mtx_};

    switch ( rpcActive_ ) {
    case None_:
      return ResultType::Continue;
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

void Client::clearCache( void )
{
  std::unique_lock<std::mutex> lck{*mtx_};
  cache_.clear();
}
