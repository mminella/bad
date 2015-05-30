#ifndef METH1_CLIENT_HH
#define METH1_CLIENT_HH

#include <vector>

#include "implementation.hh"

#include "address.hh"
#include "file.hh"
#include "socket.hh"

/**
 * Stratergy 1.
 * - No upfront work.
 * - Full linear scan for each record.
 */
namespace meth1 {

  /**
   * Client defines the coordinator that comminicates with a set of ndoes to
   * retrieve their data and merge them to form a sorted file.
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
    Client( Client && other ) = delete;
    Client & operator=( const Client && other ) = delete;

  private:
    void DoInitialize( void );
    std::vector<Record> DoRead( size_type pos, size_type size );
    size_type DoSize( void );
  };

}

#endif /* METH1_CLIENT_HH */
