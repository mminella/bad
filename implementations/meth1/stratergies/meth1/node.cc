#include <fcntl.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <vector>

#include "node.hh"
#include "priority_queue.hh"

#include "record.hh"

#include "address.hh"
#include "event_loop.hh"
#include "exception.hh"
#include "file.hh"
#include "socket.hh"

using namespace std;
using namespace meth1;

// Check size_t for a vector is a size_type or larger
static_assert( sizeof( vector<Record>::size_type ) >=
                 sizeof( Implementation::size_type ),
               "Vector<Record> max size is too small!" );

/* Construct Node */
Node::Node( string file, string port, size_type max_mem )
  : data_{file.c_str(), O_RDONLY}
  , port_{port}
  , last_{Record::MIN}
  , fpos_{0}
  , size_{0}
  , max_mem_{max_mem}
{
}

/* run the node - list and respond to RPCs */
void Node::Run( void )
{
  TCPSocket sock{IPV4};
  sock.set_reuseaddr();
  sock.bind( {"0.0.0.0", port_} );
  sock.listen();

  while ( true ) {
    try {
      auto client = sock.accept();

      while ( true ) {
        string str = client.read( 1 );
        if ( client.eof() ) {
          break;
        }

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
  string str = client.read( 2 * sizeof( size_type ), true );
  size_type pos = *( reinterpret_cast<const size_type *>( str.c_str() ) );
  size_type amt = *( reinterpret_cast<const size_type *>( str.c_str() ) + 1 );

  // perform read
  vector<Record> recs = Read( pos, amt );
  size_type siz = recs.size();

  // serialize results to wire
  client.write( reinterpret_cast<const char *>( &siz ), sizeof( size_type ) );
  for ( auto const & r : recs ) {
    client.write( r.str( Record::NO_LOC ) );
  }
}

void Node::RPC_Size( TCPSocket & client )
{
  size_type siz = Size();
  client.write( reinterpret_cast<const char *>( &siz ), sizeof( size_type ) );
}

/* Initialization routine */
void Node::DoInitialize( void ) { return; }

/* Read a contiguous subset of the file starting from specified position. */
vector<Record> Node::DoRead( size_type pos, size_type size )
{
  Record after = seek( pos );

  // read the records from disk
  auto recs = linear_scan( after, size );
  if ( recs.size() > 0 ) {
    last_ = recs.back();
    fpos_ = pos + recs.size();
  }

  return recs;
}

/* Return the the number of records on disk */
Node::size_type Node::DoSize( void )
{
  if ( size_ == 0 ) {
    linear_scan( Record::MIN );
  }
  return size_;
}

/* seek returns the record that occurs just before `pos` so it can be passed to
 * `linear_scan` */
Record Node::seek( size_type pos )
{
  cout << "- Seek (" << pos << ")" << endl;
  auto start = chrono::high_resolution_clock::now();

  Record after{Record::MIN};

  if ( fpos_ == pos ) {
    // continuing from last time
    after = last_;
  } else {
    // remember, retrieving the record just before `pos`
    for ( size_type i = 0; i < pos; i += max_mem_ ) {
      auto recs = linear_scan( after, min( pos - i, max_mem_ ) );
      if ( recs.size() == 0 ) {
        break;
      }
      after = recs.back();
    }
  }

  // seek timings
  auto end = chrono::high_resolution_clock::now();
  auto dur = chrono::duration_cast<chrono::milliseconds>( end - start ).count();
  cout << "* Seek took: " << dur << "ms" << endl;

  return after;
}

/* Perform a full linear scan to return the next smallest record that occurs
 * after the 'after' record. */
vector<Record> Node::linear_scan( const Record & after, size_type size )
{
  static uint64_t pass = 0;

  cout << "- Linear scan " << pass++ << " (" << size << ")" << endl;
  auto start = chrono::high_resolution_clock::now();

  // TODO: Better to use pointers to Record? Or perhaps to change Record to
  // heap allocate?
  mystl::priority_queue<Record> recs{size + 1};

  size_type i;

  for ( i = 0;; i++ ) {
    const char * rec_str = get<0>( data_.internal_read( Record::SIZE, true ) );
    Record next = Record::ParseRecord( rec_str, i, false );
    if ( data_.eof() ) {
      break;
    }

    if ( after < next ) {
      if ( recs.size() < size ) {
        recs.push( next.clone() );
      } else if ( next < recs.top() ) {
        recs.push( next.clone() );
        recs.pop();
      }
    }
  }

  // timings -- scan
  auto split = chrono::high_resolution_clock::now();
  auto dur   = chrono::duration_cast<chrono::milliseconds>( split - start ).count();
  cout << "* Linear scan took: " << dur << "ms" << endl;

  // cache number of records on disk
  size_ = i;

  // rewind the file
  data_.rewind();

  // sort final heap
  auto vrecs = recs.container();
  sort_heap( vrecs.begin(), vrecs.end() );

  // timings -- sort
  auto end = chrono::high_resolution_clock::now();
  dur = chrono::duration_cast<chrono::milliseconds>( end - split ).count();
  cout << "* In-memory sort took: " << dur << "ms" << endl;

  // timings -- total
  dur = chrono::duration_cast<chrono::milliseconds>( end - start ).count();
  cout << "* Read took: " << dur << "ms" << endl;

  return vrecs;
}
