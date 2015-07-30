#include <algorithm>
#include <cassert>
#include <system_error>
#include <vector>
#include <utility>

#include "address.hh"
#include "buffered_io.hh"
#include "exception.hh"
#include "file.hh"
#include "socket.hh"
#include "timestamp.hh"
#include "util.hh"

#include "record.hh"

#include "node.hh"
#include "overlapped_io.hh"

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
  , recio_{data_}
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
      // EOF
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

inline uint64_t Node::rec_sort( vector<Record> & recs ) const
{
  auto t0 = time_now();
#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
  string_sort( recs.begin(), recs.end() );
#else
  sort( recs.begin(), recs.end() );
#endif
  return time_diff<ms>( t0 );
}

Record Node::seek( uint64_t pos )
{
  auto t0 = time_now();

  // can we continue from last time?
  if ( pos == 0 ) {
    last_ = Record::MIN;
  } else if ( fpos_ != pos ) {
    // remember, retrieving the record just before `pos`
    for ( uint64_t i = 0; i < pos; i += max_mem_ ) {
      auto recs = linear_scan( Record{Record::MIN}, min( pos - i, max_mem_ ) );
      if ( recs.size() == 0 ) {
        break;
      }
      last_ = recs.back();
    }
  }

  auto tt = time_diff<ms>( t0 );
  cout << "[Seek (" << pos << ")]: " << tt << "ms" << endl;
  return last_;
}

/* Perform a full linear scan to return the next smallest record that occurs
 * after the 'after' record. */
vector<Record> Node::linear_scan( const Record & after, uint64_t size )
{
  static uint64_t pass = 0;
  uint64_t p = pass++;

  auto t0 = time_now();
  tdiff_t tmerge = 0, tsort = 0, tplace = 0;

  recio_.rewind();

  // TWEAK: sort + merge block size...
  uint64_t block_size = max( size / 2, (uint64_t) 1 );


  if ( size == 1 ) {
    // optimized case: size = 1
    auto min = Record( Record::MAX );
    for ( uint64_t i = 0;; i++ ) {
      const char * r = recio_.next_record();
      if ( r == nullptr ) {
        break;
      }
      RecordPtr next{r, i};
      if ( next < min and next > after ) {
        min = Record( r, i );
      }
    }
    return {min};
  } else {
    // general case: size = n
    vector<Record> vnext, vpast, vin;
    vnext.reserve( size );
    vin.reserve( block_size );

    uint64_t i = 0;
    for ( bool run = true; run; ) {
      auto tp0 = time_now();
      for ( ; vin.size() < block_size; i++ ) {
        const char * r = recio_.next_record();
        if ( r == nullptr ) {
          run = false;
          break;
        }
        RecordPtr next{r, i};
        if ( next > after ) {
          vin.emplace_back( r, i );
        }
      }
      tplace += time_diff<ms>( tp0 );

      // optimize for some cases
      if ( vin.size() > 0 ) {
        tsort += rec_sort( vin );
        if ( vpast.size() == 0 ) {
          swap( vpast, vin );
        } else {
          vnext.resize( min( size, vin.size() + vpast.size() ) );
          tmerge += move_merge( vin, vpast, vnext );
          swap( vnext, vpast );
          vin.clear();
        }
      }
    }

    auto tt = time_diff<ms>( t0 );
    cout << "[Linear scan (" << p << ")]: " << tt << "ms" << endl;
    cout << "[Place total]: " << tplace << "ms" << endl;
    cout << "[Sort total]: " << tsort << "ms" << endl;
    cout << "[Merge total]: " << tmerge << "ms" << endl;

    return vpast;
  }
}
