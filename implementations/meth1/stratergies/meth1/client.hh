#ifndef METH1_CLIENT_HH
#define METH1_CLIENT_HH

#include <vector>

#include "implementation.hh"

#include "address.hh"
#include "file.hh"
#include "socket.hh"

/**
 * Strategy 1.
 * - No upfront work.
 * - Full linear scan for each record.
 */
namespace meth1 {

  /**
   * Client defines the coordinator that communicates with a single node.
   */
  class Client : public Implementation {
  private:
    TCPSocket sock_;
    Address addr_;

  public:
    /* constructor */
    Client(Address node);

    /* destructor */
    ~Client() = default;

    /* forbid copy & move assignment */
    Client( const Client & other ) = delete;
    Client & operator=( const Client & other ) = delete;
    Client( Client && other );
    Client & operator=( Client && other );

    /* provide each side of RPC so we can multi-cast */
    void sendRead( size_type pos, size_type size );
    std::vector<Record> recvRead( void );
    void sendSize( void );
    size_type recvSize( void );

  private:
    void DoInitialize( void );
    std::vector<Record> DoRead( size_type pos, size_type size );
    size_type DoSize( void );
  };

}

#endif /* METH1_CLIENT_HH */
