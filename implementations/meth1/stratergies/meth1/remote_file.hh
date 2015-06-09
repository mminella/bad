#ifndef REMOTE_FILE_HH
#define REMOTE_FILE_HH

#include <vector>

#include "client.hh"
#include "implementation.hh"

#include "address.hh"

/**
 * Strategy 1.
 * - No upfront work.
 * - Full linear scan for each record.
 */
namespace meth1
{

/**
 * RemoteFile is a helper wrapper around a Client that handles performing
 * read-ahead for better performance. It also presents closer to a traditional,
 * stateful file interface that can make some algorithms easier.
 */
class RemoteFile
{
public:
  using size_type = Implementation::size_type;

private:
  Client client_;       // contained client
  size_type fpos_;      // file position
  size_type cached_;    // records cached (read-ahead)
  size_type readahead_; // read-ahead amount
  size_type low_cache_; // low-cache amount to start read-ahead
  bool eof_;

public:
  RemoteFile( Client c, size_type readahead, size_type low_cache = 0 );

  RemoteFile( Address node, size_type readahead, size_type low_cache = 0 )
    : RemoteFile( Client{node}, readahead, low_cache )
  {
  }

  void open( void );
  void seek( size_type offset );
  void prefetch( size_type size = 0 );
  std::vector<Record> read( void );
  std::vector<Record> peek( void );
  void next( void );
  size_type stat( void );

  Poller::Action RPCRunner( void );
};
}

#endif /* REMOTE_FILE_HH */
