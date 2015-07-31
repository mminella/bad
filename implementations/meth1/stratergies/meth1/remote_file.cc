#include <vector>

#include "address.hh"
#include "poller.hh"

#include "record.hh"

#include "client.hh"
#include "remote_file.hh"

using namespace std;
using namespace meth1;

RemoteFile::RemoteFile( Client c, uint64_t readahead, uint64_t low_cache )
  : client_{move( c )}
  , data_{}
  , dpos_{0}
  , fpos_{0}
  , cached_{0}
  , readahead_{readahead}
  // TWEAK: read-ahead amount
  , low_cache_{readahead / 2}
  , eof_{false}
{
  if ( low_cache != 0 ) {
    low_cache_ = low_cache;
  }
  if ( low_cache >= readahead ) {
    throw runtime_error( "low-cache mark must be less than read-ahead" );
  }
}

Poller::Action RemoteFile::RPCRunner( void )
{
  return client_.RPCRunner();
}

void RemoteFile::open( void )
{
  client_.Initialize();
}

void RemoteFile::seek( uint64_t offset )
{
  if ( offset != fpos_ ) {
    fpos_ = offset;
    data_ = {};
    dpos_ = 0; 
    cached_ = 0;
    eof_ = false;
    client_.clearCache();
  }
}

void RemoteFile::prefetch()
{
  client_.prepareRead( fpos_ + cached_, readahead_ );
  cached_ += readahead_;
}

void RemoteFile::read_ahead( void )
{
  if ( cached_ <= low_cache_ ) {
    prefetch();
  }
}

void RemoteFile::next( void )
{
  if ( !eof_ ) {
    cached_--;
    fpos_++;
    read_ahead();
  }
}

Record * RemoteFile::peek( void )
{
  if ( eof_ ) {
    return nullptr;
  }

  if ( data_.size() == 0 or fpos_ - dpos_ >= data_.size() ) {
    data_ = client_.Read( fpos_, readahead_ );
    dpos_ = fpos_;
    if ( data_.size() == 0 ) {
      eof_ = true;
      return nullptr;
    }
  }

  return &data_[fpos_ - dpos_];
}

Record * RemoteFile::read( void )
{
  if ( eof_ ) {
    return nullptr;
  }
  read_ahead();
  auto r = peek();
  if ( r != nullptr ) {
    next();
  }
  return r;
}

vector<Record> RemoteFile::read_chunk( void )
{
  if ( eof_ ) {
    return {};
  }

  if ( data_.size() == 0 ) {
    data_ = client_.Read( fpos_, readahead_ );
    dpos_ = fpos_;
    if ( data_.size() == 0 ) {
      eof_ = true;
      return {};
    }
  }

  if ( dpos_ == fpos_ ) {
    fpos_ += data_.size();
    if ( cached_ >= data_.size() ) {
      cached_ -= data_.size();
    }
    read_ahead();
    return move( data_ );
  } else {
    throw runtime_error( "can't read chunk when not on bounary" );
  }
}

uint64_t RemoteFile::size( void )
{
  return client_.Size();
}
