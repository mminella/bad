#ifndef METH4_CLUSTER_MAP_HH
#define METH4_CLUSTER_MAP_HH

#include <string>

#include "address.hh"
#include "file.hh"

#include "record.hh"

#include "config_file.hh"
#include "meth4_knobs.hh"

class ClusterMap
{
public:
  /* Minimum number of buckets to have per disk */
  static constexpr size_t MIN_BKTS_PER_DISK = Knobs4::MIN_BUCKETS_PER_DISK;

private:
  struct shard_t {
    uint16_t id_;
    uint8_t k_[Rec::KEY_LEN];
    shard_t( uint16_t id ) : id_{id} {}
  };

  using shards_t = std::vector<shard_t>;
  using pre_shards_t = std::vector<shards_t>;

  /* cluster config */
  ConfigFile config_;
  std::vector<File> recFiles_;
  std::vector<std::string> diskPaths_;
  size_t disks_;
  size_t recordsLocally_;
  size_t bucketsPerNode_;

  /* actual cached bucket mapping */
  shards_t shards_;
  pre_shards_t preShards_;

  /* bucket to local node mapping */
  std::vector<uint16_t> myBuckets_;

  /* helper functions */
  shards_t calculateShards( size_t buckets ) const noexcept;
  pre_shards_t precomputeFirstByte( shards_t shards ) const noexcept;
  std::vector<uint16_t> calcMyBuckets( uint16_t id, size_t nodes, size_t disks,
                                       size_t buckets ) const noexcept;

public:
  ClusterMap( std::string hostname, std::string configFile,
    std::vector<std::string> dataFiles );

  /* My ID (and position in vector) in the cluster. */
  uint16_t myID( void ) const noexcept;

  /* The list of buckets this node is responsible for. */
  const std::vector<uint16_t> & myBuckets( void ) const noexcept;

  /* Map a global bucket number to a local number that can be used to offset
   * into an array of all buckets for this node. */
  size_t bucket_local_id( uint16_t bkt ) const noexcept;

  /* Map a global bucket number to a disk on the local machine. */
  uint8_t bucket_disk( uint16_t bkt ) const noexcept;

  /* map a bucket (or key) to a node. */
  uint16_t bucket_node( uint16_t bkt ) const noexcept;

  /* Files to open. */
  std::vector<File> & files( void ) noexcept;

  /* Disks paths for local node. */
  const std::vector<std::string> & disk_paths( void ) const noexcept;

  /* Bucket directory for a particular disk. */
  std::string disk_bucket_directory( size_t diskID ) const noexcept;

  /* Path to a bucket file. */
  std::string bucket_path( uint16_t bkt ) const noexcept;

  /* Path to bucket once sorted. */
  std::string sorted_bucket_path( uint16_t bkt ) const noexcept;

  /* Number of local disks available. */
  size_t disks( void ) const noexcept;

  /* List of addresses (including SELF) in the cluster. */
  std::vector<Address> addresses( void ) const noexcept;

  /* cluster sizes. */
  size_t nodes( void ) const noexcept;

  /* total number of buckets in cluster. */
  size_t buckets( void ) const noexcept;

  /* map a key to a bucket. */
  uint16_t bucket( const uint8_t * key ) const noexcept;
};

#endif /* METH4_CLUSTER_MAP_HH */
