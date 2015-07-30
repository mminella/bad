#ifndef METH1_CLIENT_HH
#define METH1_CLIENT_HH

#include <chrono>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

#include "address.hh"
#include "buffered_io.hh"
#include "file.hh"
#include "poller.hh"
#include "socket.hh"
#include "timestamp.hh"

#include "implementation.hh"

/**
 * Strategy 1.
 * - No upfront work.
 * - Full linear scan for each record.
 */
namespace meth1
{

/**
 * Client communicates with a single backend node. Performs some basic caching
 * of records read from the network, mostly to allow asynchronous IO.
 */
class Client : public Implementation
{
private:
  /* how many records to read from the network in one go */
  static constexpr size_t MAX_CACHED_EXTENTS = 2;

  /* A cache of records with starting position recorded */
  struct Buffer {
    std::vector<Record> records;
    uint64_t fpos;
    Buffer( std::vector<Record> r, uint64_t fp )
      : records{r}
      , fpos{fp}
    {
    }
    bool operator<( const Buffer & b ) const { return fpos < b.fpos; }
  };

  /* network state */
  BufferedIO_O<TCPSocket> sock_;
  Address addr_;

  /* rpc state */
  enum RPC { Read_, Size_, None_ } rpcActive_;
  uint64_t rpcPos_;
  uint64_t rpcSize_;
  clk::time_point rpcStart_;

  /* cache of buffer extents */
  std::list<Buffer> cache_;

  /* file size (cache result) */
  uint64_t size_;

  /* synchronization for read vs write */
  std::unique_ptr<std::mutex> mtx_;
  std::unique_ptr<std::condition_variable> cv_;

  /* methods */
  void DoInitialize( void );
  std::vector<Record> DoRead( uint64_t pos, uint64_t size );
  uint64_t DoSize( void );

  void waitOnRPC( std::unique_lock<std::mutex> & lck );
  bool fillFromCache( std::vector<Record> & recs, uint64_t & pos,
                      uint64_t & size );

  void sendRead( std::unique_lock<std::mutex> & lck, uint64_t pos,
                 uint64_t size );
  void recvRead( void );
  void sendSize( std::unique_lock<std::mutex> & lck );
  void recvSize( void );

public:
  Client( Address node );

  /* Returns an action to be run in a event loop for handling RPC */
  Poller::Action RPCRunner( void );

  /* Setup a read to execute asynchronously, when done it'll be placed in the
   * Client's internal cache and available through 'Read' */
  void prepareRead( uint64_t pos, uint64_t size )
  {
    std::unique_lock<std::mutex> lck{*mtx_};
    sendRead( lck, pos, size );
  }

  /* Setup a size call to execute asynchronously, when done it'll be available
   * through the 'Size' method. */
  void prepareSize( void )
  {
    std::unique_lock<std::mutex> lck{*mtx_};
    sendSize( lck );
  }

  void clearCache( void );
};
}

#endif /* METH1_CLIENT_HH */
