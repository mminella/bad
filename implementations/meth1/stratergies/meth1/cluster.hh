#ifndef METH1_CLUSTER2_HH
#define METH1_CLUSTER2_HH

#include <vector>

#include "address.hh"
#include "file.hh"

#include "client.hh"
#include "node.hh"

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
  static uint64_t constexpr MEM_RESERVE = Node::MEM_RESERVE;
  static uint64_t constexpr MAX_BUF_SIZE = MEM_RESERVE * uint64_t( 5 ); // 5GB
  static uint64_t constexpr WRITE_BUF_N = 2;

  std::vector<Client> clients_;
  uint64_t chunkSize_;
  uint64_t bufSize_;
  uint64_t memFree_;

public:
  Cluster( std::vector<Address> nodes, uint64_t chunkSize = 0 );

  uint64_t Size( void );
  Record ReadFirst( void );
  void Read( uint64_t pos, uint64_t size );
  void ReadAll( void );
  void WriteAll( File out );
};
}

#endif /* METH1_CLUSTER2_HH */
