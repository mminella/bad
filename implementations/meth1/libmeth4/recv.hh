#ifndef METH4_RECV_HH
#define METH4_RECV_HH

#include <vector>
#include <utility>

#include "address.hh"
#include "poller.hh"
#include "socket.hh"

#include "block.hh"
#include "cluster_map.hh"
#include "disk_writer.hh"
#include "meth4_knobs.hh"

/* Handle receiving data from a single node in the cluster. Will receive data
 * for all buckets. */
class NetIn
{
private:
  /* RPC Format: <uint16_t, uint64_t> = <bucket_id, rpc_size> */
  static constexpr size_t HDRSIZE = sizeof( uint16_t ) + sizeof( uint64_t );

  /* FSM for the connection with a backend */
  enum wire_state_t { IDLE, HEADER, PARSE, BODY, DONE };

  ClusterMap & cluster_;
  std::vector<DiskWriter> & disks_;
  size_t bucketsLive_;

  TCPSocket sock_;
  wire_state_t wireState_;
  uint8_t header_[HDRSIZE];
  size_t headerOnWire_;
  uint16_t bucketOnWire_;
  size_t bucketLocalID_;
  size_t bodyOnWire_;

  bool cantMove_;

public:
  NetIn( ClusterMap & cluster, std::vector<DiskWriter> & disks,
         TCPSocket sock );

  /* allow move */
  NetIn( NetIn && other );

  /* disable copy */
  NetIn( const NetIn & ) = delete;

  /* Hack: we disable the ability to move, called once we take a reference. */
  void disableMove( void ) noexcept { cantMove_ = true; }

  TCPSocket & socket( void ) noexcept { return sock_; }

  /* bool indicates if NetIn is still active */
  bool read( std::vector<block_t> & buckets );
};

class Receiver
{
public:
  static constexpr bool NET_NON_BLOCKING = Knobs4::NET_NON_BLOCKING;
  static constexpr size_t DISK_BLOCK_SIZE =
    Knobs4::DISK_W_BLOCK_SIZE * Rec::SIZE;

  static_assert( DISK_BLOCK_SIZE % Rec::SIZE == 0,
    "DISK_BLOCK_SIZE not a multiple of Rec::SIZE");

private:
  ClusterMap & cluster_;
  Poller poll_;
  TCPSocket sock_;
  std::vector<NetIn> netins_;
  std::vector<block_t> buckets_;
  size_t backendsLive_;
  std::vector<DiskWriter> disks_;

public:
  Receiver( ClusterMap & cluster, Address address );
  ~Receiver( void );

  /* Disable copy & move */
  Receiver( const Receiver & ) = delete;
  Receiver( Receiver && ) = delete;
  Receiver & operator=( const Receiver & ) = delete;
  Receiver & operator=( Receiver && ) = delete;

  /* Handle the network receive side */
  void waitForConnections( void );
  void receiveLoop( void );
  void waitFinished( void );
};

#endif /* METH4_RECV_HH */
