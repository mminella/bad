#include <fcntl.h>

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>

#include "node.hh"

#include "record.hh"

#include "address.hh"
#include "event_loop.hh"
#include "exception.hh"
#include "file.hh"
#include "socket.hh"

using namespace std;
using namespace meth1;

// Check size_t for a vector is a size_type or larger
static_assert(
  sizeof( std::vector<Record>::size_type )
    >= sizeof( Implementation::size_type ),
  "Vector<Record> max size is too small!"
);

/* Construct Node */
Node::Node( string file, string port )
  : data_ { file.c_str(), O_RDONLY }
  , port_ { port }
  , last_ { Record::MIN }
  , fpos_ { 0 }
  , size_ { 0 }
{
}

/* run the node - list and respond to RPCs */
void Node::Run( void )
{
  TCPSocket sock;
  sock.set_reuseaddr();
  sock.bind( { "::0", port_ } );
  sock.listen();

  while ( true ) {
    try {
      auto client = sock.accept();

      while ( true ) {
        string str = client.read(1);
        if ( client.eof() ) { break; }

        switch ( str[0] ) {
        case 0: // Read
          RPC_Read( client );
          break;
        case 1: // Size
          RPC_Size( client );
          break;
        default:
          // TODO: throw error!
          break;
        }
      }
    } catch ( const exception & e ) {
      print_exception( e );
    }
  }
}

void Node::RPC_Read( TCPSocket & client )
{
  // parse arguments
  string str = client.read( 2 * sizeof(size_type) );
  size_type pos = *( reinterpret_cast<const size_type *>( str.c_str() ) );
  size_type amt = *( reinterpret_cast<const size_type *>( str.c_str() ) + 1 );

  // perform read
  vector<Record> recs = Read( pos, amt );
  size_type siz = recs.size();

  // serialize results to wire
  client.write( reinterpret_cast<const char *>( &siz ), sizeof(size_type) );
  for ( auto & r : recs ) {
    client.write( r.str( Record::WITH_LOC ) );
  }
}

void Node::RPC_Size( TCPSocket & client )
{
  size_type siz = Size();
  client.write( reinterpret_cast<const char *>( &siz ), sizeof(size_type) );
}

/* Initialization routine */
void Node::DoInitialize( void )
{
  return;
}

/* Read a contiguous subset of the file starting from specified position. */
std::vector<Record> Node::DoRead( size_type pos, size_type size )
{
  // establish starting record
  Record after { Record::MIN };
  size_type fpos { 0 };
  if ( fpos_ == pos ) {
    after = last_;
    fpos = fpos_;
  }

  std::vector<Record> recs;
  recs.reserve( size );

  // forward scan through all records until we hit the subset we want.
  for ( ; fpos < pos + size; fpos++ ) {
    after = linear_scan( data_, after );
    // collect records
    if ( fpos >= pos ) {
      recs.push_back( after );
    }
  }

  fpos_ = fpos;
  last_ = after;

  return recs;
}

/* Return the the number of records on disk */
Node::size_type Node::DoSize( void )
{
  if ( size_ == 0 ) {
    linear_scan( data_, Record::MIN);
  }
  return size_;
}

/* Perform a full linear scan to return the next smallest record that occurs
 * after the 'after' record. */
Record Node::linear_scan( File & in, const Record & after )
{
  Record min { Record::MAX };
  size_type i;

  for ( i = 0; ; i++ ) {
    Record next = Record::ParseRecord( in.read( Record::SIZE ), i, false );
    if (in.eof()) {
      break;
    }

    int cmp = next.compare(after);
    if ( cmp >= 0 && next < min ) {
      // we check the diskloc to ensure we don't pick up same key, but can still
      // handle duplicate keys.
      if ( cmp > 0 or after.diskloc() < next.diskloc() ) {
        min = next.clone();
      }
    }
  }

  // cache number of records on disk
  size_ = i;

  // rewind the file
  in.rewind();

  return min;
}
