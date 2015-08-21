#ifndef METH1_NODE_HH
#define METH1_NODE_HH

#include <mutex>
#include <condition_variable>
#include <utility>

#include "buffered_io.hh"
#include "file.hh"
#include "overlapped_rec_io.hh"
#include "raw_vector.hh"
#include "socket.hh"
#include "timestamp.hh"

#include "implementation.hh"
#include "record.hh"

/**
 * Stratergy 1.
 * - No upfront work.
 * - Full linear scan for each record.
 */
namespace meth1
{

/**
 * Node defines the backend running on each node with a subset of the data.
 */
class Node : public Implementation2
{
private:
  File data_;
  OverlappedRecordIO<Rec::SIZE> recio_;
  std::string port_;
  Record last_;
  uint64_t fpos_;
  uint64_t max_mem_;
  uint64_t lpass_;

public:
  Node( std::string file, std::string port, uint64_t max_memory,
        bool odirect = false );

  /* No copy or move */
  Node( const Node & n ) = delete;
  Node & operator=( const Node & n ) = delete;
  Node( Node && n ) = delete;
  Node & operator=( Node && n ) = delete;

  /* Run the node - list and respond to RPCs */
  void Run( void );

private:
  void DoInitialize( void );
  RecV DoRead( uint64_t pos, uint64_t size );
  uint64_t DoSize( void );

  Record seek( uint64_t pos );

  RecV linear_scan( const Record & after, uint64_t size = 1 );
  RecV linear_scan_one( const Record & after );
  RecV linear_scan_pq( const Record & after, uint64_t size );
  RecV linear_scan_chunk( const Record & after, uint64_t size );
  RecV linear_scan_chunk2( const Record & after, uint64_t size,
                           RR * r1, RR * r2, RR *r3, uint64_t r1x );
  RecV linear_scan_chunk2( const Record & after, uint64_t size );

  void RPC_Read( BufferedIO_O<TCPSocket> & client );
  void RPC_Size( BufferedIO_O<TCPSocket> & client );
};
}

#endif /* METH1_NODE_HH */
