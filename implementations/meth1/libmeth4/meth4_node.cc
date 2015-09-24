#include <algorithm>
#include <chrono>
#include <memory>
#include <vector>
#include <utility>

#include "exception.hh"
#include "file.hh"
#include "sync_print.hh"
#include "timestamp.hh"
#include "util.hh"

#include "cluster_map.hh"
#include "config_file.hh"
#include "meth4_knobs.hh"
#include "recv.hh"
#include "send.hh"
#include "sort.hh"

using namespace std;

void debug_cluster_map( ClusterMap & cluster )
{
  print( "myid", cluster.myID() );
  print( "memory", memory_exists() );
  print( "disks", cluster.disks() );
  for ( auto & d : cluster.disk_paths() ) {
    print( "disk-path", d );
  }
  print( "in-files", cluster.files().size() );
  size_t total = 0;
  for ( auto & f : cluster.files() ) {
    size_t fsize = f.size();
    total += fsize;
    print( "file-size", fsize );
  }
  print( "total-size", total );
  print( "bucket-size", cluster.bucketMaxSize() );
  print( "mybuckets", cluster.myBuckets().size() );
  for ( auto & b : cluster.myBuckets() ) {
    print( "local-bucket", b, cluster.bucket_local_id( b ),
      (size_t) cluster.bucket_disk( b ) );
  }
  print( "buckets", cluster.buckets() );
  for ( size_t i = 0; i < cluster.buckets(); i++ ) {
    print( "bucket", i, cluster.bucket_node( i ) );
  }
  print( "nodes", cluster.nodes() );
  for ( auto & a : cluster.addresses() ) {
    print( "backend", a.to_string() );
  }
}

// Do in seperate block as we want destructors to run to free memory after
// phase one is complete.
void phase_one( ClusterMap & cluster, string port )
{
  // startup cluster
  Receiver receiver( cluster, {"0.0.0.0", port} );

  // wait short while for server socket to come up
  this_thread::sleep_for( chrono::seconds( Knobs4::STARTUP_WAIT ) );

  // establish outbound connections (separate thread)
  NetOut net( cluster );

  // establish inbound connections
  receiver.waitForConnections();

  // transfer data to correct nodes
  vector<unique_ptr<Sender>> senders;
  for ( auto & f : cluster.files() ) {
    senders.emplace_back( new Sender( f, cluster, net ) );
    senders.back()->start();
  }
  receiver.receiveLoop();
}

void phase_two( ClusterMap & cluster )
{
  Sorter sorter( cluster );
}

void run( size_t nodeID, string port, string conffile, vector<string> files )
{
  print( "start", timestamp<ms>() );

  // configure cluster
  ClusterMap cluster( nodeID, conffile, files );
  debug_cluster_map( cluster );

  // shard data into buckets
  auto t0 = time_now();
  print( "phase-one-start", timestamp<ms>() );
  phase_one( cluster, port );
  print( "phase-one-end", timestamp<ms>(), time_diff<ms>( t0 ) );

  // sort each bucket
  auto t1 = time_now();
  print( "phase-two-start", timestamp<ms>() );
  phase_two( cluster );
  print( "phase-two-end", timestamp<ms>(), time_diff<ms>( t1 ) );

  print( "finish", timestamp<ms>(),
    time_diff<ms>( t0 ) - Knobs4::STARTUP_WAIT * 1000 );

  // give chance for other nodes to finish before exit (useful while testing)
  this_thread::sleep_for( chrono::seconds( 2 ) );
}

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc < 5 ) {
    // pass in hostname rather than retrieve to allow easy testing
    throw runtime_error( "Usage: " + string( argv[0] )
      + " [node id] [port] [config file] [data files...]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    run( atoll( argv[1] ), argv[2], argv[3], {argv+4, argv+argc} );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
