#ifndef METH2_CLUSTER2_HH
#define METH2_CLUSTER2_HH

#include <vector>

#include "address.hh"
#include "file.hh"

#include "client.hh"

/**
 * Strategy 2.
 */
namespace meth2
{

/**
 * Cluster defines the coordinator that communicates with a set of nodes to
 * retrieve their data and merge them to form a sorted file.
 */
class Cluster
{
private:
  std::vector<Client> clients_;
  uint64_t chunkSize_;

public:
  Cluster( std::vector<Address> nodes, uint64_t chunkSize );

  uint64_t Size( void );
  Record ReadFirst( void );
  void Read( uint64_t pos, uint64_t size );
  void ReadAll( void );
  void WriteAll( File out );
};
}

#endif /* METH2_CLUSTER2_HH */
