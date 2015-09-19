#include <iostream>

#include "buffered_io.hh"
#include "exception.hh"
#include "socket.hh"
#include "timestamp.hh"

#include "record.hh"

#include "client.hh"

using namespace std;
using namespace meth2;

Client::Client( Address node )
  : sock_{{(IPVersion)(node.domain())}}
  , addr_{node}
  , rpcStart_{}
  , rpcPos_{0}
{
  sock_.io().connect( addr_ );
}

void Client::sendRead( uint64_t pos, uint64_t siz )
{
  static int pass = 0;
  //rpcStart_ = time_now();
  rpcPos_ = pos;

  //cout << "start-read, " << pass++ << ", " << pos << ", " << siz << endl;

  char data[1 + 2 * sizeof( uint64_t )];
  data[0] = 0;
  *reinterpret_cast<uint64_t *>( data + 1 ) = pos;
  *reinterpret_cast<uint64_t *>( data + 1 + sizeof( uint64_t ) ) = siz;
  sock_.io().write_all( data, sizeof( data ) );
}

uint64_t Client::recvRead( void )
{
  static int pass = 0;

  auto nrecsStr = sock_.read_buf_all( sizeof( uint64_t ) ).first;
  //auto split = time_now();
  //cout << "read, " << pass << ", " << time_diff<ms>( split, rpcStart_ ) << 
  //endl;

  uint64_t nrecs = *reinterpret_cast<const uint64_t *>( nrecsStr );
  //cout << "recv-read, " << pass << ", " << rpcPos_ << ", " << nrecs << endl;
  return nrecs;
}

RecordPtr Client::readRecord( void )
{
  auto recStr = sock_.read_buf_all( Rec::SIZE ).first;
  return { recStr };
}

void Client::sendIRead( uint64_t pos, uint64_t siz )
{
  static int pass = 0;
  //rpcStart_ = time_now();
  rpcPos_ = pos;

  //cout << "start-iread, " << pass++ << ", " << pos << ", " << siz << endl;

  char data[1 + 2 * sizeof( uint64_t )];
  data[0] = 1;
  *reinterpret_cast<uint64_t *>( data + 1 ) = pos;
  *reinterpret_cast<uint64_t *>( data + 1 + sizeof( uint64_t ) ) = siz;
  sock_.io().write_all( data, sizeof( data ) );
}

uint64_t Client::recvIRead( void )
{
  static int pass = 0;

  auto nrecsStr = sock_.read_buf_all( sizeof( uint64_t ) ).first;
  //auto split = time_now();
  //cout << "iread, " << pass << ", " << time_diff<ms>( split, rpcStart_ ) << 
  //endl;

  uint64_t nrecs = *reinterpret_cast<const uint64_t *>( nrecsStr );
  //cout << "recv-iread, " << pass << ", " << rpcPos_ << ", " << nrecs << endl;
  return nrecs;
}

RecordLoc Client::readIRecord( void )
{
  //= sock_.read_buf_all( Rec::KEY_LEN + 3*sizeof(uint64_t) ).first;
  RecordLoc recLoc;
  recLoc.read(sock_);
  return recLoc;
}

void Client::sendSize( void )
{
  //rpcStart_ = time_now();
  int8_t rpc = 2;
  sock_.io().write_all( (char *)&rpc, 1 );
}

uint64_t Client::recvSize( void )
{
  static int pass = 0;

  auto sizeStr = sock_.read_buf_all( sizeof( uint64_t ) ).first;
  //cout << "size, " << pass++ << ", " << time_diff<ms>( rpcStart_ ) << endl;

  uint64_t size = *reinterpret_cast<const uint64_t *>( sizeStr );
  //cout << "recv-size, " << pass << ", " << size << endl;
  return size;
}

