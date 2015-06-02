#ifndef METH1_CLUSTER_HH
#define METH1_CLUSTER_HH

#include <vector>

#include "implementation.hh"

#include "client.hh"

#include "address.hh"
#include "file.hh"
#include "socket.hh"

/**
 * Strategy 1.
 * - No upfront work.
 * - Full linear scan for each record.
 */
namespace meth1
{

/**
 * Cluster defines the coordinator that communicates with a set of nodes to
 * retrieve their data and merge them to form a sorted file.
 */
class Cluster : public Implementation
{
private:
  std::vector<Client> nodes_;

public:
  Cluster( std::vector<Address> nodes );

private:
  void DoInitialize( void );
  std::vector<Record> DoRead( size_type pos, size_type size );
  size_type DoSize( void );
};
}

#endif /* METH1_CLUSTER_HH */
