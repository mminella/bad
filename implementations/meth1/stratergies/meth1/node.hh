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

  // for REUSE_MEM
  size_t gr1x = 0;
  size_t gr2x = 0;
  RR * gr1 = nullptr;
  RR * gr2 = nullptr;
  RR * gr3 = nullptr;

  void free_buffers( RR * r1, RR * r3, size_t size );

public:
  Node( std::vector<std::string> files, std::string port,
        bool odirect = false);

  /* No copy or move */
  Node( const Node & n ) = delete;
  Node & operator=( const Node & n ) = delete;
  Node( Node && n ) = delete;
  Node & operator=( Node && n ) = delete;

  ~Node( void )
  {
    if ( gr1 != nullptr and gr3 != nullptr ) {
      free_buffers( gr1, gr3, gr2x );
    }
    if ( gr2 != nullptr ) {
      delete[] gr2;
    }
  }

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
