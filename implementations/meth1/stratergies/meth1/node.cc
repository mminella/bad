#include <algorithm>
#include <cassert>
#include <chrono>
#include <system_error>
#include <vector>

#include "address.hh"
#include "buffered_io.hh"
#include "exception.hh"
#include "file.hh"
#include "socket.hh"

#include "record.hh"

#include "node.hh"
#include "overlapped_io.hh"
#include "priority_queue.hh"

#include "config.h"

using namespace std;
using namespace meth1;

#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
#include <boost/sort/spreadsort/string_sort.hpp>
using namespace boost::sort::spreadsort;
#endif

/* Construct Node */
Node::Node( string file, string port, uint64_t max_mem )
  : data_{file.c_str(), O_RDONLY}
#if OVERLAP_IO == 0
  , bio_{data_}
#endif
  , port_{port}
  , last_{Record::MIN}
  , fpos_{0}
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
      BufferedIO_O<TCPSocket> client {sock.accept()};

      while ( true ) {
        const char * str = client.read_buf_all( 1 ).first;
        if ( client.eof() ) {
          break;
        }
        switch ( str[0] ) {
        case 0:
          RPC_Read( client );
          break;
        case 1:
          RPC_Size( client );
          break;
        default:
          throw runtime_error( "Unknown RPC method: " + to_string(str[0]) );
          break;
        }
      }
    } catch ( const exception & e ) {
      print_exception( e );
    }
  }
}

void Node::RPC_Read( BufferedIO_O<TCPSocket> & client )
{
  const char * str = client.read_buf_all( 2 * sizeof( uint64_t ) ).first;
  uint64_t pos = *( reinterpret_cast<const uint64_t *>( str ) );
  uint64_t amt = *( reinterpret_cast<const uint64_t *>( str ) + 1 );

  vector<Record> recs = Read( pos, amt );
  uint64_t siz = recs.size();

  client.write_all( reinterpret_cast<const char *>( &siz ), sizeof( uint64_t ) );
  for ( auto const & r : recs ) {
    r.write( client );
  }
  client.flush( true );
}

void Node::RPC_Size( BufferedIO_O<TCPSocket> & client )
{
  uint64_t siz = Size();
  client.write_all( reinterpret_cast<const char *>( &siz ), sizeof( uint64_t ) );
  client.flush( true );
}

void Node::DoInitialize( void ) { return; }

vector<Record> Node::DoRead( uint64_t pos, uint64_t size )
{
  auto t0 = chrono::high_resolution_clock::now();

  Record after = seek( pos );
  auto recs = rec_sort( linear_scan( after, size ) );

  auto t1 = chrono::high_resolution_clock::now();
  auto tt = chrono::duration_cast<chrono::milliseconds>( t1 - t0 ).count();
  cout << "* Read took: " << tt << "ms" << endl;

  if ( recs.size() > 0 ) {
    last_ = recs.back();
    fpos_ = pos + recs.size();
  }
  return recs;
}

uint64_t Node::DoSize( void )
{
  return data_.size() / Record::SIZE;
}

vector<Record> Node::rec_sort( mystl::priority_queue<Record> recs )
{
  auto t0 = chrono::high_resolution_clock::now();

  vector<Record> vrecs {move( recs.container() )};
#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
  string_sort( vrecs.begin(), vrecs.end() );
#else
  sort( vrecs.begin(), vrecs.end() );
#endif

  // timings -- sort
  auto t1 = chrono::high_resolution_clock::now();
  auto tt = chrono::duration_cast<chrono::milliseconds>( t1 - t0 ).count();
  cout << "* In-memory sort took: " << tt << "ms" << endl;

  return vrecs;
}

Record Node::seek( uint64_t pos )
{
  cout << "- Seek (" << pos << ")" << endl;
  auto t0 = chrono::high_resolution_clock::now();

  Record after{Record::MIN};

  if ( fpos_ == pos ) {
    // continuing from last time
    after = last_;
  } else {
    // remember, retrieving the record just before `pos`
    for ( uint64_t i = 0; i < pos; i += max_mem_ ) {
      auto recs = linear_scan( after, min( pos - i, max_mem_ ) );
      if ( recs.size() == 0 ) {
        break;
      }
      after = recs.top();
    }
  }

  auto t1 = chrono::high_resolution_clock::now();
  auto tt = chrono::duration_cast<chrono::milliseconds>( t1 - t0 ).count();
  cout << "* Seek took: " << tt << "ms" << endl;

  return after;
}

/* Perform a full linear scan to return the next smallest record that occurs
 * after the 'after' record. */
mystl::priority_queue<Record>
Node::linear_scan( const Record & after, uint64_t size )
{
  auto t0 = chrono::high_resolution_clock::now();

  static uint64_t pass = 0;
  cout << "- Linear scan " << pass++ << " (" << size << ")" << endl;

#if OVERLAP_IO == 1
  OverlappedRecordIO<Record::SIZE> recio( data_ );
  recio.start();
#endif

  mystl::priority_queue<Record> recs{size + 1};
  for ( uint64_t i = 0;; i++ ) {
#if OVERLAP_IO == 1
    const char * r = recio.next_record();
    if ( r == nullptr ) { break; }
#else
    const char * r = bio_.read_buf_all( Record::SIZE ).first;
    if ( data_.eof() ) { break; }
#endif

    Record next = Record( r, i );
    if ( after < next ) {
      if ( recs.size() < size ) {
        recs.push( next );
      } else if ( next < recs.top() ) {
        recs.push( next );
        recs.pop();
      }
    }
  }

  data_.rewind();

  auto t1 = chrono::high_resolution_clock::now();
  auto tt = chrono::duration_cast<chrono::milliseconds>( t1 - t0 ).count();
  cout << "* Linear scan took: " << tt << "ms" << endl;

  return recs;
}

