#include <algorithm>
#include <cassert>
#include <chrono>
#include <system_error>
#include <vector>
#include <utility>

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
  : data_{file.c_str(), O_RDONLY} // O_DIRECT
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
  auto recs = linear_scan( seek( pos ), size );
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

inline void Node::rec_sort( vector<Record> & recs ) const
{
  auto t0 = chrono::high_resolution_clock::now();

#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
  string_sort( recs.begin(), recs.end() );
#else
  sort( recs.begin(), recs.end() );
#endif

  auto t1 = chrono::high_resolution_clock::now();
  auto tt = chrono::duration_cast<chrono::milliseconds>( t1 - t0 ).count();
  cout << "[Sort]: " << tt << "ms" << endl;
}

Record Node::seek( uint64_t pos )
{
  auto t0 = chrono::high_resolution_clock::now();

  // can we continue from last time?
  if ( fpos_ != pos ) {
    // remember, retrieving the record just before `pos`
    for ( uint64_t i = 0; i < pos; i += max_mem_ ) {
      auto recs = linear_scan( Record{Record::MIN}, min( pos - i, max_mem_ ) );
      if ( recs.size() == 0 ) {
        break;
      }
      last_ = recs.back();
    }
  }

  auto t1 = chrono::high_resolution_clock::now();
  auto tt = chrono::duration_cast<chrono::milliseconds>( t1 - t0 ).count();
  cout << "[Seek (" << pos << ")]: " << tt << "ms" << endl;
  return last_;
}

/* A merge that move's elements rather than copy and doesn't have a stupid API
 * like STL */
template <class T>
void move_merge ( vector<T> & in1, vector<T> & in2, vector<T> & out )
{
  auto t0 = chrono::high_resolution_clock::now();

  T *s1 = in1.data(), *s2 = in2.data(), *rs = out.data();
  T *e1 = s1 + in1.size(), *e2 = s2 + in2.size(), *re = rs + out.capacity();

  while ( s1 != e1 and s2 != e2 and rs != re ) {
    *rs++ = move( (*s2<*s1)? *s2++ : *s1++ );
  }

  if ( rs != re ) {
    if ( s1 != e1 ) {
      move( s1, s1 + min( re - rs, e1 - s1 ), rs );
    } else if ( s2 != e2 ) {
      move( s2, s2 + min( re - rs, e2 - s2 ), rs );
    }
  }

  auto t1 = chrono::high_resolution_clock::now();
  auto tt = chrono::duration_cast<chrono::milliseconds>( t1 - t0 ).count();
  cout << "[Merge]: " << tt << "ms" << endl;
}

/* Perform a full linear scan to return the next smallest record that occurs
 * after the 'after' record. */
vector<Record> Node::linear_scan( const Record & after, uint64_t size )
{
  static uint64_t pass = 0;

  auto t0 = chrono::high_resolution_clock::now();
  OverlappedRecordIO<Record::SIZE> recio( data_ );
  recio.start();

  // sort + merge block size...
  size_t block_size = size / 2;

  vector<Record> vnext, vpast, vin;
  vnext.reserve( size );
  vin.reserve( block_size );

  uint64_t i = 0;
  for ( bool run = true; run; ) {
    for ( ; vin.size() < block_size; i++ ) {
      const char * r = recio.next_record();
      if ( r == nullptr ) {
        run = false;
        break;
      }
      RecordPtr next{r, i};
      if ( next > after ) {
        vin.emplace_back( r, i );
      }
    }

    // optimize for some cases
    if ( vin.size() > 0 ) {
      rec_sort( vin );
      if ( vpast.size() == 0 ) {
        swap( vpast, vin );
      } else {
        vnext.resize( min( size, vin.size() + vpast.size() ) );
        move_merge( vin, vpast, vnext );
        swap( vnext, vpast );
        vin.clear();
      }
    }
  }

  data_.rewind();

  auto t1 = chrono::high_resolution_clock::now();
  auto tt = chrono::duration_cast<chrono::milliseconds>( t1 - t0 ).count();
  cout << "[Linear scan (" << pass++ << ")]: " << tt << "ms" << endl;

  return vpast;
}

