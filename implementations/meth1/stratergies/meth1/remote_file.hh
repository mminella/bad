#ifndef REMOTE_FILE_HH
#define REMOTE_FILE_HH

#include <vector>

#include "address.hh"
#include "poller.hh"

#include "record.hh"

#include "implementation.hh"
#include "client.hh"

/**
 * Strategy 1.
 * - No upfront work.
 * - Full linear scan for each record.
 */
namespace meth1
{

/**
 * RemoteFile is a helper wrapper around a Client presents a stateful File
 * interface and performs read-ahead. */
class RemoteFile
{
private:
  Client client_;      // contained client
  uint64_t fpos_;      // file position
  uint64_t cached_;    // records cached (read-ahead)
  uint64_t readahead_; // read-ahead amount
  uint64_t low_cache_; // low-cache amount to start read-ahead
  bool eof_;           // end-of-file hit?

  void read_ahead( void );

public:
  RemoteFile( Client c, uint64_t readahead, uint64_t low_cache = 0 );

  RemoteFile( Address node, uint64_t readahead, uint64_t low_cache = 0 )
    : RemoteFile( Client{node}, readahead, low_cache )
  {
  }

  void open( void );
  void seek( uint64_t offset );
  void prefetch( uint64_t size = 0 );
  void next( void );
  std::vector<Record> peek( void );
  std::vector<Record> read( void );
  uint64_t stat( void );

  Poller::Action RPCRunner( void );
};
}

#endif /* REMOTE_FILE_HH */
