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
  struct NodeSplit {
      NodeSplit() : clientNo(0), size(0),
		    start(0), end(0), n(0) {}
      void dump() {
	  std::cout << "start=" << start <<
		       ", end=" << end <<
		       ", n=" << n << std::endl;
      }
      uint32_t clientNo;
      uint64_t size;
      uint64_t start;
      uint64_t end;
      uint64_t n;
      RecordLoc key;
      RecordLoc nextKey;
  };

  Cluster( std::vector<Address> nodes, uint64_t chunkSize );
  ~Cluster();
  uint64_t Size( void );
  std::vector<NodeSplit> GetSplit(uint64_t n);
  Record ReadFirst( void );
  void Read( uint64_t pos, uint64_t size );
  void ReadAll( void );
  void WriteAll( File out );
private:
  uint64_t Size( Client &c );
  std::vector<RecordLoc> IRead( Client &c, uint64_t pos, uint64_t size );
  uint64_t IBSearch( Client &c, RecordLoc &rl);
};
}

#endif /* METH2_CLUSTER2_HH */
