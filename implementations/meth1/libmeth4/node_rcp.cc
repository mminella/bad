#include "exception.hh"
#include "timestamp.hh"
#include "sync_print.hh"

#include "meth4_knobs.hh"
#include "node_rcp.hh"

using namespace std;

NodeRCP::NodeRCP( ClusterMap & cluster, Address address )
  : cluster_{cluster}
  , sock_{IPV4}
  , rdy_{}
  , thread_{}
  , ready_{false}
{
  sock_.set_reuseaddr();
  sock_.set_nodelay();
  sock_.set_send_buffer( Knobs4::NET_SND_BUF );
  sock_.set_recv_buffer( Knobs4::NET_RCV_BUF );
  sock_.bind( address );
  sock_.listen();

  thread_ = thread( &NodeRCP::handleClient, this );
}

NodeRCP::~NodeRCP( void )
{
  thread_.join();
}

void NodeRCP::notifyReady( void )
{
  rdy_.send( true );
}

void NodeRCP::handleClient( void )
{
  while ( true ) {
    auto client = sock_.accept();
    if (not ready_) {
      // wait for phase one to finish
      ready_ = rdy_.recv();
    }
    try {
      while ( true ) {
        auto str = client.read_all( 1 );
        if ( client.eof() ) {
          break;
        }
        switch ( str[0] ) {
        case RPC::BUCKETS:
          for ( auto b : cluster_.myBuckets() ) {
            print( "bucket-size", b, cluster_.bucketSize( b ) );
          }
          break;
        case RPC::EXIT:
          print( "\nexit", timestamp<ms>() );
          return;
        default:
          throw runtime_error( "Unknown RPC method: " + to_string(str[0]) );
          break;
        }
      }
    } catch ( const exception & e ) {
      // EOF
    }
  }
  print( "\ndirty-exit", timestamp<ms>() );
}
