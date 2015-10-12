#ifndef NODE_RCP_HH
#define NODE_RCP_HH

#include <thread>

#include "address.hh"
#include "channel.hh"
#include "socket.hh"

#include "cluster_map.hh"

class NodeRCP
{
public:
  enum RPC : int8_t {
    BUCKETS,
    SORT,
    EXIT
  };

private:
  ClusterMap & cluster_;
  TCPSocket sock_;
  Channel<bool> rdy_;
  std::thread thread_;
  bool ready_;

  void handleClient( void );

public:
  NodeRCP( ClusterMap & cluster, Address address );

  NodeRCP( const NodeRCP & ) = delete;
  NodeRCP & operator=( const NodeRCP & ) = delete;
  NodeRCP( NodeRCP && ) = delete;
  NodeRCP & operator=( NodeRCP && ) = delete;

  ~NodeRCP( void );

  void notifyReady( void );
};

#endif /* NODE_RCP_HH */
