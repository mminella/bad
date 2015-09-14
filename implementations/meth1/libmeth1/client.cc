#include <iostream>

#include "buffered_io.hh"
#include "exception.hh"
#include "socket.hh"
#include "sync_print.hh"
#include "timestamp.hh"

#include "record.hh"

#include "client.hh"

using namespace std;
using namespace meth1;

Client::Client( Address node )
  : sock_{(IPVersion)(node.domain())}
  , addr_{node}
  , rpcStart_{}
  , rpcPos_{0}
  , sendPass_{0}
  , recvPass_{0}
  , sizePass_{0}
{
  sock_.set_nodelay();
  sock_.set_send_buffer( Knobs::NET_SND_BUF );
  sock_.set_recv_buffer( Knobs::NET_RCV_BUF );
  sock_.connect( addr_ );
}

void Client::sendRead( uint64_t pos, uint64_t siz )
{
  rpcStart_ = time_now();
  rpcPos_ = pos;

  print( "read-start", sock_.fd_num(), ++sendPass_, pos, siz, timestamp<ms>() );

  char data[1 + 2 * sizeof( uint64_t )];
  data[0] = 0;
  *reinterpret_cast<uint64_t *>( data + 1 ) = pos;
  *reinterpret_cast<uint64_t *>( data + 1 + sizeof( uint64_t ) ) = siz;
  sock_.write_all( data, sizeof( data ) );
}

uint64_t Client::recvRead( void )
{
  print( "read-wait", sock_.fd_num(), ++recvPass_, time_diff<ms>( rpcStart_ ) );

  char data[sizeof( uint64_t )];
  char * nrecsStr = data;
  sock_.read_all( nrecsStr, sizeof( uint64_t ) );
  uint64_t nrecs = *reinterpret_cast<uint64_t *>( nrecsStr );

  print( "read", sock_.fd_num(), recvPass_, rpcPos_, nrecs,
    time_diff<ms>( rpcStart_ ) );

  return nrecs;
}

void Client::sendSize( void )
{
  rpcStart_ = time_now();
  int8_t rpc = 1;
  sock_.write_all( (char *)&rpc, 1 );
}

uint64_t Client::recvSize( void )
{
  char data[sizeof( uint64_t )];
  char * sizeStr = data;
  sock_.read_all( sizeStr, sizeof( uint64_t ) );
  uint64_t size = *reinterpret_cast<const uint64_t *>( sizeStr );

  print( "size", sock_.fd_num(), ++sizePass_, size,
    time_diff<ms>( rpcStart_ ) );

  return size;
}

void Client::sendShutdown( void )
{
  int8_t rpc = 2;
  sock_.write_all( (char *) &rpc, 1 );
}
