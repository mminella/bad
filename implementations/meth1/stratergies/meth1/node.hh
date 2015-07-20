#ifndef METH1_NODE_HH
#define METH1_NODE_HH

#include <mutex>
#include <condition_variable>

#include "buffered_io.hh"
#include "file.hh"
#include "socket.hh"

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
class Node : public Implementation
{
private:
  BufferedIO_O<File> data_;
  std::string port_;
  Record last_;
  uint64_t fpos_;
  uint64_t max_mem_;

  static constexpr size_t BLOCK = 4096;
  static constexpr size_t NBLOCKS = 10;
  static constexpr size_t BUF_SIZE = NBLOCKS * BLOCK;

  char buf_[BUF_SIZE];
  char * wptr_ = buf_;
  char * rptr_ = buf_;
  size_t blocks_ = 0;
  size_t last_block_ = 0;

  std::mutex mtx_;
  std::condition_variable cv_;

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

  Record seek( uint64_t pos );
  std::vector<Record> linear_scan( const Record & after, uint64_t size = 1 );

  void RPC_Read( BufferedIO_O<TCPSocket> & client );
  void RPC_Size( BufferedIO_O<TCPSocket> & client );

  void read_file( void );
  char * chunk( size_t & );
  bool next_chunk( void );

};
}

#endif /* METH1_NODE_HH */
