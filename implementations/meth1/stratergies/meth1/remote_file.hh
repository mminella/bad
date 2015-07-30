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
 * interface and performs read-ahead.
 */
class RemoteFile
{
private:
  Client client_;            // contained client
  std::vector<Record> data_; // current spot in file
  uint64_t dpos_;            // data position
  uint64_t fpos_;            // file position
  uint64_t cached_;          // data_.Size() + outstanding read RPCs
  uint64_t readahead_;       // read-ahead amount
  uint64_t low_cache_;       // low-cache amount to start read-ahead
  bool eof_;                 // end-of-file hit?

  void read_ahead( void );

public:
  RemoteFile( Client c, uint64_t readahead, uint64_t low_cache = 0 );

  RemoteFile( Address node, uint64_t readahead, uint64_t low_cache = 0 )
    : RemoteFile( Client{node}, readahead, low_cache )
  {
  }

  Poller::Action RPCRunner( void );
  Client & client( void ) { return client_; }

  void open( void );
  void seek( uint64_t offset );
  void prefetch();
  void next( void );
  Record * peek( void );
  Record * read( void );
  uint64_t size( void );
};
}

#endif /* REMOTE_FILE_HH */
