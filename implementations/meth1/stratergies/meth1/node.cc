#include <algorithm>
#include <cassert>
#include <system_error>
#include <vector>
#include <utility>

#include "address.hh"
#include "buffered_io.hh"
#include "exception.hh"
#include "file.hh"
#include "overlapped_rec_io.hh"
#include "socket.hh"
#include "timestamp.hh"
#include "util.hh"

#include "record.hh"

#include "node.hh"
#include "priority_queue.hh"

#include "config.h"

#define USE_PQ 0

using namespace std;
using namespace meth1;

#ifdef HAVE_BOOST_SORT_SPREADSORT_STRING_SORT_HPP
#include <boost/sort/spreadsort/string_sort.hpp>
using namespace boost::sort::spreadsort;
#endif

/* Construct Node */
Node::Node( string file, string port, uint64_t max_mem, bool odirect )
  : data_{file.c_str(), odirect ? O_RDONLY | O_DIRECT : O_RDONLY}
  , recio_{data_}
  , port_{port}
  , last_{Rec::MIN}
  , fpos_{0}
  , max_mem_{max_mem}
  , lpass_{0}
{
}

void Node::DoInitialize( void ) { return; }

/* Run the node - list and respond to RPCs */
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

  vector<Record> recs;
  if ( pos < Size() ) {
    recs = Read( pos, amt );
  }

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

vector<Record> Node::DoRead( uint64_t pos, uint64_t size )
{
  static size_t pass = 0;

  auto t0 = time_now();
  auto recs = linear_scan( seek( pos ), size );
  if ( recs.size() > 0 ) {
    last_ = recs.back();
    fpos_ = pos + recs.size();
  }
  cout << "do-read, " << ++pass << ", " << time_diff<ms>( t0 ) << endl;
  cout << endl;
  cout << endl;

  return recs;
}

uint64_t Node::DoSize( void )
{
  return data_.size() / Rec::SIZE;
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

/* Return the record that corresponds to the specified position. */
Record Node::seek( uint64_t pos )
{
  // can we continue from last time?
  if ( pos == 0 ) {
    last_ = Rec::MIN;
  } else if ( pos >= Size() ) {
    last_ = Rec::MAX;
  } else if ( fpos_ != pos ) {
    // remember, retrieving the record just before `pos`
    for ( uint64_t i = 0; i < pos; i += max_mem_ ) {
      auto recs = linear_scan( Record{Rec::MIN}, min( pos - i, max_mem_ ) );
      if ( recs.size() == 0 ) {
        break;
      }
      last_ = recs.back();
    }
  }
  return last_;
}

/* Perform a single linear scan of the file, returning the next `size` smallest
 * records that occur directly after the `after` record. */
vector<Record> Node::linear_scan( const Record & after, uint64_t size )
{
  cout << "linear_scan, " << ++lpass_ << ", " << timestamp<ms>()
    << ", start" << endl;
  if ( size == 1 ) {
    return linear_scan_one( after );
  } else {
#if USE_PQ == 1
    return linear_scan_pq( after, size );
#else
    return linear_scan_chunk( after, size );
#endif
  }
}

/* Linear scan optimized for retrieving just one record. */
vector<Record> Node::linear_scan_one( const Record & after )
{
  auto t0 = time_now();
  auto min = Record( Rec::MAX );

  recio_.rewind();

  for ( uint64_t i = 0;; i++ ) {
    const char * r = recio_.next_record();
    if ( r == nullptr ) {
      break;
    }
    RecordPtr next{r, i};
    if ( next > after and min > next ) {
      min = Record( r, i );
    }
  }

  auto tt = time_diff<ms>( t0 );
  cout << "linear scan, " << lpass_ << ", " << tt << endl;
  return {min};
}

/* Linear scan using a priority queue for sorting. */
vector<Record> Node::linear_scan_pq( const Record & after, uint64_t size )
{
  auto t0 = time_now();
  tdiff_t tsort = 0, tplace = 0;
  size_t rrs = 0, cmps = 0, pushes = 0, pops = 0;

  recio_.rewind();
  mystl::priority_queue<Record> pq{size+1};
  uint64_t i = 0;

  while ( true ) {
    const char * r = recio_.next_record();
    if ( r == nullptr ) {
      break;
    }
    RecordPtr next{r, i};
    rrs++;
    cmps++;
    if ( next > after ) {
      if ( pq.size() < size ) {
        pushes++;
        pq.emplace( r, i );
      } else if ( next < pq.top() ) {
        cmps++; pushes++; pops++;
        pq.emplace( r, i );
        pq.pop();
      } else {
        cmps++;
      }
    }
  }
  tplace += time_diff<ms>( t0 );

  vector<Record> vrecs = move( pq.container() );
  tsort += rec_sort( vrecs );

  auto tt = time_diff<ms>( t0 );
  cout << "insert, "      << lpass_ << ", " << tplace << endl;
  cout << "sort, "        << lpass_ << ", " << tsort  << endl;
  cout << "linear scan, " << lpass_ << ", " << tt     << endl;
  cout << endl;
  cout << "recs, "        << lpass_ << ", " << rrs    << endl;
  cout << "cmps, "        << lpass_ << ", " << cmps   << endl;
  cout << "pushes, "      << lpass_ << ", " << pushes << endl;
  cout << "pops, "        << lpass_ << ", " << pops   << endl;
  cout << "size, "        << lpass_ << ", " << size   << endl;
  return vrecs;
}

/* Linear scan using a chunked sorting + merge strategy. */
vector<Record> Node::linear_scan_chunk( const Record & after, uint64_t size )
{
  auto t0 = time_now();
  tdiff_t tsort = 0, tplace = 0, tmerge = 0;

  recio_.rewind();

  // TWEAK: sort + merge block size...
  uint64_t blk_size = max( uint64_t(1), size / 4 );
  vector<Record> vnext, vpast, vin;
  vnext.reserve( size );
  vin.reserve( blk_size );

  uint64_t i = 0;
  for ( bool run = true; run; ) {
    auto tp0 = time_now();
    for ( ; vin.size() < blk_size; i++ ) {
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
  cout << "insert, "      << lpass_ << ", " << tplace << endl;
  cout << "sort, "        << lpass_ << ", " << tsort  << endl;
  cout << "merge, "       << lpass_ << ", " << tmerge << endl;
  cout << "linear scan, " << lpass_ << ", " << tt     << endl;
  cout << endl;

  return vpast;
}
