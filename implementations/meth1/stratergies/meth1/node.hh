#ifndef METH1_NODE_HH
#define METH1_NODE_HH

#include "buffered_io.hh"
#include "file.hh"
#include "overlapped_rec_io.hh"
#include "raw_vector.hh"
#include "socket.hh"
#include "timestamp.hh"

#include "record.hh"

/* Sorting strategy to use? Ordered slowest to fastest. */
#define USE_PQ 0
#define USE_CHUNK 1

/* We can use a move or copy strategy -- the copy is actaully a little better
 * as we play some tricks to ensure we reuse allocations as much as possible.
 * With copy we use `size + r1x` value memory, but with move, we use up to
 * `2.size + r1x`. */
#define USE_COPY 1

/* We can reuse our sort+merge buffers for a big win! Be careful though, as the
 * results returned by scan are invalidate when you next call scan. */
#define REUSE_MEM 1

/* Use a parallel merge implementation? */
#define TBB_PARALLEL_MERGE 1

/* Amount of memory to reserve for OS and other tasks */
#define MEMRESERVE 1024 * 1024 * 1000

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
class Node
{
public:
  static uint64_t constexpr MEM_RESERVE = MEMRESERVE;

  using RR = RecordS;
  using RecV = RawVector<RR>;

private:
  File data_;
  OverlappedRecordIO<Rec::SIZE> recio_;
  std::string port_;
  Record last_;
  uint64_t fpos_;
  uint64_t seek_chunk_;
  uint64_t lpass_;

public:
  Node( std::string file, std::string port, bool odirect = false );

  /* No copy or move */
  Node( const Node & n ) = delete;
  Node & operator=( const Node & n ) = delete;
  Node( Node && n ) = delete;
  Node & operator=( Node && n ) = delete;

  /* Run the node - list and respond to RPCs */
  void Run( void );

  /* API */
  void Initialize( void );
  RecV Read( uint64_t pos, uint64_t size );
  uint64_t Size( void );

private:
  Record seek( uint64_t pos );

  RecV linear_scan( const Record & after, uint64_t size = 1 );
  RecV linear_scan_one( const Record & after );
  RecV linear_scan_pq( const Record & after, uint64_t size );
  RecV linear_scan_chunk( const Record & after, uint64_t size,
                          RR * r1, RR * r2, RR *r3, uint64_t r1x );
  RecV linear_scan_chunk( const Record & after, uint64_t size );

  void RPC_Read( BufferedIO_O<TCPSocket> & client );
  void RPC_Size( BufferedIO_O<TCPSocket> & client );
};
}

#endif /* METH1_NODE_HH */
