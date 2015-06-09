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
  , eof_{false}
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
  if ( offset < fpos_ ) {
    cached_ = 0;
    eof_ = false;
  } else if ( offset < fpos_ + cached_ ) {
    cached_ -= offset - fpos_;
  } else if ( !eof_ ) {
    cached_ = 0;
  }

  fpos_ = offset;
}

void RemoteFile::prefetch( size_type size )
{
  if ( cached_ > 0 ) {
    return;
  }

  if ( size == 0 || size > readahead_ ) {
    size = readahead_;
  }
  client_.prepareRead( fpos_, size );
  cached_ = size;
}

void RemoteFile::read_ahead( void )
{
  // advance prefetch if low
  if ( cached_ <= low_cache_ ) {
    client_.prepareRead( fpos_ + cached_, readahead_ );
    cached_ += readahead_;
  }
}

void RemoteFile::next( void )
{
  if ( !eof_ ) {
    cached_--;
    fpos_++;
  }

  read_ahead();
}

vector<Record> RemoteFile::peek( void )
{
  if ( eof_ ) {
    return {};
  }

  // read next record
  vector<Record> rec = client_.Read( fpos_, 1 );
  if ( rec.size() == 0 ) {
    eof_ = true;
  }

  return rec;
}

vector<Record> RemoteFile::read( void )
{
  if ( eof_ ) {
    return {};
  }

  read_ahead();

  vector<Record> rec = peek();
  if ( rec.size() > 0 ) {
    next();
  }

  return rec;
}

Poller::Action RemoteFile::RPCRunner( void ) { return client_.RPCRunner(); }

RemoteFile::size_type RemoteFile::stat( void ) { return client_.Size(); }
