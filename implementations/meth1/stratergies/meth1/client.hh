#ifndef METH1_CLIENT_HH
#define METH1_CLIENT_HH

#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

#include "implementation.hh"

#include "address.hh"
#include "exception.hh"
#include "file.hh"
#include "poller.hh"
#include "socket.hh"

/**
 * Strategy 1.
 * - No upfront work.
 * - Full linear scan for each record.
 */
namespace meth1
{

/**
 * Client defines the coordinator that communicates with a single node.
 */
class Client : public Implementation
{
private:
  static constexpr size_t MAX_CACHED_EXTENTS = 3;

  /* A cache of records with starting position recorded */
  struct Buffer {
    std::vector<Record> records;
    size_type fpos;
    Buffer( std::vector<Record> r, size_type fp )
      : records{r}
      , fpos{fp}
    {
    }
    bool operator<( const Buffer & b ) const { return fpos < b.fpos; }
  };

  /* network state */
  TCPSocket sock_;
  Address addr_;

  /* rpc state */
  enum RPC { Read_, Size_, None_ } rpcActive_;
  size_type rpcPos_;

  /* cache of buffer extents, kept sorted by buffer offset */
  std::vector<Buffer> cache_;

  /* lru of buffer extents for eviction -- we use the fpos for an id, so
   * invariants is no two extents have the same fpos */
  std::list<size_type> lru_;

  /* cache file size */
  size_type size_;

  /* synchronization for read vs write */
  std::unique_ptr<std::mutex> mtx_;
  std::unique_ptr<std::condition_variable> cv_;

  /* methods */
  void DoInitialize( void );
  std::vector<Record> DoRead( size_type pos, size_type size );
  size_type DoSize( void );

  void checkNoRPC( void );
  bool fillFromCache( std::vector<Record> & recs, size_type & pos,
                      size_type & size );

  void sendRead( size_type pos, size_type size );
  std::vector<Record> recvRead( void );
  void sendSize( void );
  void recvSize( void );

public:
  Client( Address node );

  /* Returns an action to be run in a event loop for handling RPC responses for
   * this client */
  Poller::Action RPCRunner( void );

  /* Setup a read to execute asynchronously, when done it'll be placed in the
   * Client's internal cache and available through 'Read' */
  void prepareRead( size_type pos, size_type size ) { sendRead( pos, size ); }

  /* Setup a size call to execute asynchronously, when done it'll be available
   * through the 'Size' method. */
  void prepareSize( void ) { sendSize(); }
};
}

#endif /* METH1_CLIENT_HH */
