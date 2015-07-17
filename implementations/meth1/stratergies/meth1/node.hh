#ifndef METH1_NODE_HH
#define METH1_NODE_HH

#include "buffered_io.hh"
#include "file.hh"
#include "socket.hh"

#include "implementation.hh"
#include "record.hh"

#include "priority_queue.hh"

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
  size_type fpos_;
  size_type max_mem_;

public:
  Node( std::string file, std::string port, size_type max_memory );

  /* run the node - list and respond to RPCs */
  void Run( void );

private:
  void DoInitialize( void );
  std::vector<Record> DoRead( size_type pos, size_type size );
  size_type DoSize( void );

  Record seek( size_type pos );
  std::vector<Record> linear_scan( const Record & after, size_type size = 1 );

  void RPC_Read( BufferedIO_O<TCPSocket> & client );
  void RPC_Size( BufferedIO_O<TCPSocket> & client );
};
}

#endif /* METH1_NODE_HH */
