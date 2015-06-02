#ifndef METH1_NODE_HH
#define METH1_NODE_HH

#include "implementation.hh"

#include "priority_queue.hh"

#include "record.hh"

#include "file.hh"
#include "socket.hh"

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
  std::string port_;
  Record last_;
  size_type fpos_;
  size_type size_;

public:
  Node( std::string file, std::string port );

  /* run the node - list and respond to RPCs */
  void Run( void );

private:
  void DoInitialize( void );
  std::vector<Record> DoRead( size_type pos, size_type size );
  size_type DoSize( void );

  std::vector<Record> linear_scan( File & in, const Record & after,
                                   size_type size = 1 );

  void RPC_Read( TCPSocket & client );
  void RPC_Size( TCPSocket & client );
};

}

#endif /* METH1_NODE_HH */
