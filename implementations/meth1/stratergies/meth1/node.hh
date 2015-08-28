#ifndef METH1_NODE_HH
#define METH1_NODE_HH

#include <string>
#include <vector>

#include "config.h"
#ifdef HAVE_TBB_TASK_GROUP_H
#include "tbb/task_group.h"
#endif

#include "buffered_io.hh"
#include "raw_vector.hh"
#include "socket.hh"

#include "record.hh"
#include "rec_loader.hh"

/* How much smaller ( 1 / SORT_MERGE_RATIO ) should the sort buffer be than the
 * merge buffer?
 */
#define SORT_MERGE_RATIO 2

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
  tbb::task_group tg_;
  std::vector<RecLoader> recios_;
  std::string port_;
  Record last_;
  uint64_t fpos_;
  uint64_t seek_chunk_;
  uint64_t lpass_;
  uint64_t size_;

public:
  Node( std::vector<std::string> files, std::string port,
        bool odirect = false);

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
  RecV linear_scan_chunk( const Record & after, uint64_t size,
                          RR * r1, RR * r2, RR *r3, uint64_t r1x );
  RecV linear_scan_chunk( const Record & after, uint64_t size );

  void RPC_Read( BufferedIO_O<TCPSocket> & client );
  void RPC_Size( BufferedIO_O<TCPSocket> & client );
};
}

#endif /* METH1_NODE_HH */
