#ifndef METH1_CLUSTER_HH
#define METH1_CLUSTER_HH

#include <vector>

#include "address.hh"
#include "channel.hh"
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
class Cluster
{
private:
  std::vector<RemoteFile> files_;
  Poller poller_;

public:
  // TWEAK: readahead
  Cluster( std::vector<Address> nodes, uint64_t readahead );

  uint64_t Size( void );
  Record ReadFirst( void );
  void ReadAll( void );
  void WriteAll( File out );
};
}

#endif /* METH1_CLUSTER_HH */
