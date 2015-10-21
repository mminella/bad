#ifndef METH4_DISK_WRITER_HH
#define METH4_DISK_WRITER_HH

#include <string>
#include <vector>

#include "buffered_io.hh"
#include "channel.hh"
#include "file.hh"
#include "timestamp.hh"

#include "block.hh"
#include "cluster_map.hh"

class DiskWriter
{
private:
  static constexpr size_t DISK_QUEUE_LENGTH = Knobs4::DISK_W_QUEUE_LENGTH;

  ClusterMap & cluster_;
  uint8_t diskID_;
  std::string diskPath_;
  std::vector<File> files_;
  Channel<block_t> queue_;
  std::thread writer_;
  bool threadStarted_;
  tpoint_t start_;

  /* Convert a global bucket ID to a disk local bucket ID */
  uint16_t diskLocalBucketID( uint16_t bucket );

  /* Read from channel and write data to disk */
  void writeLoop( void );

public:
  DiskWriter( ClusterMap & cluster, uint8_t diskID, std::string diskPath );

  /* Disable copy */
  DiskWriter( const DiskWriter & ) = delete;
  DiskWriter & operator=( const DiskWriter & ) = delete;

  /* Allow move construct but not assignment */
  DiskWriter( DiskWriter && other );
  DiskWriter & operator=( DiskWriter && ) = delete;

  ~DiskWriter( void );

  /* Start the writing thread */
  void start( void );

  /* Queue a block to be written to disk */
  void send( block_t block );

  /* Wait until disk writer has drained the current queue */
  void waitDrained( bool fsync = false );
};

#endif /* METH4_DISK_WRITER_HH */
