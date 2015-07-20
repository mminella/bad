#ifndef METH1_CLUSTER_HH
#define METH1_CLUSTER_HH

#include <vector>

#include "address.hh"
#include "file.hh"
#include "poller.hh"

#include "implementation.hh"
#include "remote_file.hh"

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
  std::vector<RemoteFile> files_;
  Poller poller_;

public:
  Cluster( std::vector<Address> nodes, uint64_t read_ahead );

private:
  void DoInitialize( void );
  std::vector<Record> DoRead( uint64_t pos, uint64_t size );
  uint64_t DoSize( void );
};
}

#endif /* METH1_CLUSTER_HH */
