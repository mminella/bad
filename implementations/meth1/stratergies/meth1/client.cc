#include <iostream>
#include <vector>

#include "implementation.hh"

#include "client.hh"

#include "socket.hh"

using namespace std;
using namespace meth1;

Client::Client(Address node)
  : sock_{}
  , addr_ { node }
{
}

Client::Client( Client && other )
  : sock_ { move( other.sock_ ) }
  , addr_ { move( other.addr_ ) }
{
}

Client & Client::operator=( Client && other )
{
  if ( this != &other ) {
    sock_ = move( other.sock_ );
    addr_ = move( other.addr_ );
  }
  return *this;
}

void Client::DoInitialize( void )
{
  sock_.connect( addr_ );
}

void Client::sendRead( size_type pos, size_type size )
{
  int8_t rpc = 0;
  sock_.write( (char *) &rpc, 1 );
  sock_.write( (char *) &pos, sizeof( size_type ) );
  sock_.write( (char *) &size, sizeof( size_type ) );
}

std::vector<Record> Client::recvRead( void )
{
  string str = sock_.read( sizeof( size_type ) );
  size_type nrecs = *reinterpret_cast<const size_type *>( str.c_str() );

  vector<Record> recs {};
  recs.reserve(nrecs);
  for ( size_type i = 0; i < nrecs; i++ ) {
    string r = sock_.read( Record::SIZE_WITH_LOC );
    recs.push_back( Record::ParseRecordWithLoc( r, true ) );
  }

  return recs;
}

void Client::sendSize( void )
{
  int8_t rpc = 1;
  sock_.write( (char *) &rpc, 1 );
}

Client::size_type Client::recvSize( void )
{
  string str = sock_.read( sizeof( size_type ) );
  return *reinterpret_cast<const size_type *>( str.c_str() );
}

vector<Record> Client::DoRead( size_type pos, size_type size )
{
  sendRead( pos, size );
  return recvRead();
}

Client::size_type Client::DoSize( void )
{
  sendSize();
  return recvSize();
}
