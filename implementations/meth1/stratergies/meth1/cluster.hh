#ifndef METH1_CLUSTER_HH
#define METH1_CLUSTER_HH

#include <vector>

#include "address.hh"
#include "file.hh"
#include "poller.hh"
#include "socket.hh"

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
  static constexpr size_type READ_AHEAD = 1000;

  Cluster( std::vector<Address> nodes, size_type read_ahead = READ_AHEAD );

private:
  void DoInitialize( void );
  std::vector<Record> DoRead( size_type pos, size_type size );
  size_type DoSize( void );
};
}

#endif /* METH1_CLUSTER_HH */
