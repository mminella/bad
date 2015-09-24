#ifndef METH4_SEND_HH
#define METH4_SEND_HH

#include <thread>
#include <tuple>
#include <vector>

#include "address.hh"
#include "channel.hh"
#include "file.hh"
#include "overlapped_rec_io.hh"
#include "socket.hh"
#include "timestamp.hh"

#include "record.hh"

#include "block.hh"
#include "meth4_knobs.hh"
#include "cluster_map.hh"

class NetOut
{
public:
  static_assert( sizeof( uint64_t ) >= sizeof( size_t ),
    "size_t is larger than uint64_t" );

  static constexpr size_t NET_QUEUE_LENGTH = Knobs4::NET_QUEUE_LENGTH;
  static constexpr size_t NET_BLOCK_SIZE = Knobs4::NET_BLOCK_SIZE * Rec::SIZE;

  static_assert( NET_BLOCK_SIZE % Rec::SIZE == 0,
    "NET_BLOCK_SIZE not a multiple of Rec::SIZE" );

private:
  std::vector<TCPSocket> sockets_;
  ClusterMap & cluster_;
  Channel<block_t> queue_;
  std::thread netsend_;

  void sendLoop( void );

public:
  explicit NetOut( ClusterMap & cluster );
  ~NetOut( void );

  void send( block_t block );
};

class Sender
{
private:
  static constexpr size_t DISK_QUEUE_LENGTH = Knobs4::DISK_R_QUEUE_LENGTH;

  OverlappedRecordIO<Rec::SIZE> rio_;
  ClusterMap & cluster_;
  NetOut & net_;
  std::vector<block_t> buckets_;
  std::thread sorter_;
  tpoint_t start_;

  void _start( void );

public:
  Sender( File & file, ClusterMap & cluster, NetOut & net );
  Sender( const Sender & ) = delete;
  Sender( Sender && ) = delete;
  ~Sender( void );

  void start( void );
  void waitFinished( void );
  void countBucketDistribution( void );
};

#endif /* METH4_SEND_HH */
