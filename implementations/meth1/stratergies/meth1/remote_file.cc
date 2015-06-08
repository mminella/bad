#include <vector>

#include "client.hh"
#include "remote_file.hh"

#include "address.hh"

using namespace std;
using namespace meth1;

RemoteFile::RemoteFile( Client c, size_type readahead, size_type low_cache )
  : client_{move( c )}
  , fpos_{0}
  , cached_{0}
  , readahead_{readahead}
  , low_cache_{readahead / 2}
{
  if ( low_cache >= readahead ) {
    throw runtime_error( "low-cache mark must be less than read-ahead" );
  }

  if ( low_cache != 0 ) {
    low_cache_ = low_cache;
  }
}

void RemoteFile::open( void ) { client_.Initialize(); }

void RemoteFile::seek( size_type offset )
{
  if ( offset != fpos_ ) {
    fpos_ = offset;
    cached_ = 0;
  }
}

void RemoteFile::prefetch( size_type size )
{
  if ( size == 0 ) {
    size = readahead_;
  }
  client_.prepareRead( fpos_, size );
  cached_ = size;
}

Record RemoteFile::read( void )
{
  // advance prefetch if low
  if ( cached_ <= low_cache_ ) {
    client_.prepareRead( fpos_ + cached_, readahead_ );
    cached_ += readahead_;
  }

  // read next record
  vector<Record> rec = client_.Read( fpos_, 1 );
  cached_--;
  fpos_++;
  return rec[0];
}

Poller::Action RemoteFile::RPCRunner( void ) { return client_.RPCRunner(); }

RemoteFile::size_type RemoteFile::stat( void ) { return client_.Size(); }
