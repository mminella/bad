#ifndef METH1_CLUSTER2_HH
#define METH1_CLUSTER2_HH

#include <vector>

#include "tune_knobs.hh"

#include "address.hh"
#include "file.hh"

#include "channel.hh"
#include "client.hh"

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
  std::vector<Client> clients_;
  uint64_t chunkSize_;
  uint64_t bufSize_;

public:
  static size_t constexpr WRITE_BUF = Knobs::CLIENT_WRITE_BUFFER;
  static size_t constexpr WRITE_BUF_N = 2;

  Cluster( std::vector<Address> nodes, uint64_t chunkSize = 0 );

  uint64_t Size( void );
  Record ReadFirst( void );
  void Read( uint64_t pos, uint64_t size );
  void ReadAll( void );
  void WriteAll( File out );
  void Shutdown( void );
};
}

#endif /* METH1_CLUSTER2_HH */
