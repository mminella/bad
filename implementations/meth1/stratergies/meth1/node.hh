#ifndef METH1_NODE_HH
#define METH1_NODE_HH

#include <mutex>
#include <condition_variable>

#include "buffered_io.hh"
#include "file.hh"
#include "socket.hh"
#include "timestamp.hh"

#include "implementation.hh"
#include "overlapped_io.hh"
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
class Node : public Implementation
{
private:
  File data_;
  OverlappedRecordIO<Record::SIZE> recio_;
  std::string port_;
  Record last_;
  uint64_t fpos_;
  uint64_t max_mem_;

public:
  Node( std::string file, std::string port, uint64_t max_memory );

  /* No copy */
  Node( const Node & n ) = delete;
  Node & operator=( const Node & n ) = delete;

  /* Allow move */
  Node( Node && n ) = delete;
  Node & operator=( Node && n ) = delete;

  /* Run the node - list and respond to RPCs */
  void Run( void );

private:
  void DoInitialize( void );
  std::vector<Record> DoRead( uint64_t pos, uint64_t size );
  uint64_t DoSize( void );

  inline tdiff_t rec_sort( std::vector<Record> & recs ) const;
  Record seek( uint64_t pos );

  std::vector<Record> linear_scan( const Record & after, uint64_t size = 1 );

  void RPC_Read( BufferedIO_O<TCPSocket> & client );
  void RPC_Size( BufferedIO_O<TCPSocket> & client );
};
}

#endif /* METH1_NODE_HH */
