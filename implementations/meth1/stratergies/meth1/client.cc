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

void Client::DoInitialize( void )
{
  sock_.connect( addr_ );
}

vector<Record> Client::DoRead( size_type pos, size_type size )
{
  // send rpc
  int8_t rpc = 0;
  sock_.write( (char *) &rpc, 1 );
  sock_.write( (char *) &pos, sizeof( size_type ) );
  sock_.write( (char *) &size, sizeof( size_type ) );

  // recv rpc
  string str = sock_.read( sizeof( size_type ) );
  size_type nrecs = *reinterpret_cast<const size_type *>( str.c_str() );

  // parse records
  vector<Record> recs {};
  recs.reserve(nrecs);
  for ( size_type i = 0; i < nrecs; i++ ) {
    string r = sock_.read( Record::SIZE_WITH_LOC );
    recs.push_back( Record::ParseRecordWithLoc( r, true ) );
  }

  return recs;
}

Client::size_type Client::DoSize( void )
{
  // send rpc
  int8_t rpc = 1;
  sock_.write( (char *) &rpc, 1 );
 
  // recv rpc
  string str = sock_.read( sizeof( size_type ) );
  return *reinterpret_cast<const size_type *>( str.c_str() );
}
