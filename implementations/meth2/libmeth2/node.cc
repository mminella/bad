#include <algorithm>
#include <cassert>
#include <system_error>
#include <vector>
#include <utility>

#include "address.hh"
#include "buffered_io.hh"
#include "exception.hh"
#include "file.hh"
#include "buffered_io.hh"
#include "linux_compat.hh"
#include "overlapped_rec_io.hh"
#include "socket.hh"
#include "timestamp.hh"
#include "util.hh"

#include "record.hh"

#include "meth1_merge.hh"
#include "node.hh"
#include "circular_aio.hh"

using namespace std;
using namespace meth2;

/* Construct Node */
Node::Node( string file, string port, uint64_t max_mem, bool odirect )
  : data_{file.c_str(), odirect ? O_RDONLY | O_DIRECT : O_RDONLY},
  recs_{},
  port_{port},
  last_{Rec::MIN},
  fpos_{0},
  max_mem_{max_mem},
  lpass_{0}
{
}

void Node::Initialize( void )
{
    // Create sorted in-memory index
    BufferedIO fdi(data_);
    size_t nrecs = data_.size() / Rec::SIZE;

    // Load records
    recs_.reserve(nrecs);
    for (uint64_t i = 0; i < nrecs; i++) {
	const uint8_t *rec = (const uint8_t *)fdi.read_buf(Rec::SIZE).first;
	if (fdi.eof()) {
	    throw runtime_error("Premature EOF");
	}
	recs_.emplace_back(rec, i * Rec::SIZE + Rec::KEY_LEN);
    }

    // Sort
    rec_sort(recs_.begin(), recs_.end());

    return;
}

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
          cout << "Client EOF" << endl;
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
      cout << "Exception: " << e.what() << endl;
      // EOF
    }
  }
}

void Node::RPC_Read( BufferedIO_O<TCPSocket> & client )
{
  const char * str = client.read_buf_all( 2 * sizeof( uint64_t ) ).first;
  uint64_t pos = *( reinterpret_cast<const uint64_t *>( str ) );
  uint64_t amt = *( reinterpret_cast<const uint64_t *>( str ) + 1 );

  cout << pos << ", " << amt << endl;

  RecV recs;
  if ( pos < Size() ) {
    recs = Read( pos, amt );
  }

  cout << recs.size() << endl;

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
  client.write_all( reinterpret_cast<const char *>( &siz ),
                    sizeof( uint64_t ) );
  client.flush( true );
}

Node::RecV Node::Read( uint64_t pos, uint64_t size )
{
  static size_t pass = 0;

  auto t0 = time_now();
  Node::RecV recs;

  if (size < 100) {
    recs = linear_scan( pos , size );
  } else {
    if (pos + size > recs_.size()) {
	size = recs_.size() - pos;
    }
    recs.reserve(size);
    Circular_AIO caio(&data_, recs_);
    caio.begin(&recs, pos, size);
    caio.wait();
  }
  if ( recs.size() > 0 ) {
    last_.copy( recs.back() );
    fpos_ = pos + recs.size();
  }
  cout << "do-read, " << ++pass << ", " << time_diff<ms>( t0 ) << endl << endl;

  return recs;
}

uint64_t Node::Size( void )
{
  return data_.size() / Rec::SIZE;
}

Node::RecV Node::linear_scan(uint64_t start, uint64_t size )
{
  auto t0 = time_now();
  std::vector<RR> recV;

  recV.reserve(size);

  cout << "linear_scan, " << ++lpass_ << ", " << timestamp<ms>()
    << ", start" << endl;

  for ( uint64_t i = 0; i < size; i++ ) {
    size_t len;
    uint8_t buf[Rec::VAL_LEN];

    if (start + i >= recs_.size()) {
      break;
    }

    len = data_.pread_all((char *)&buf, Rec::VAL_LEN, recs_[start+i].loc());
    assert(len == Rec::VAL_LEN);

    recV.emplace_back(recs_[start+i], buf);
  }

  auto tt = time_diff<ms>( t0 );
  cout << "linear scan, " << lpass_ << ", " << tt << endl;

  return recV;
}

