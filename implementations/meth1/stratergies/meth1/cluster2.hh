#ifndef METH1_CLUSTER2_HH
#define METH1_CLUSTER2_HH

#include <vector>

#include "address.hh"
#include "file.hh"

#include "client2.hh"

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
class Cluster2
{
private:
  std::vector<Client2> clients_;
  uint64_t chunkSize_;

public:
  Cluster2( std::vector<Address> nodes, uint64_t chunkSize );

  uint64_t Size( void );
  Record ReadFirst( void );
  void Read( uint64_t pos, uint64_t size );
  void ReadAll( void );
  void WriteAll( File out );
};
}

#endif /* METH1_CLUSTER2_HH */
