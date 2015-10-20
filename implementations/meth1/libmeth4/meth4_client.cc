#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>

#include "address.hh"
#include "exception.hh"
#include "socket.hh"
#include "sync_print.hh"
#include "timestamp.hh"

#include "config_file.hh"
#include "meth4_client.hh"
#include "meth4_knobs.hh"

using namespace std;

void runNode( TCPSocket node, size_t i )
{
  constexpr size_t bufSize = 1024 * 1024 * 2;
  unique_ptr<char> buf( new char[bufSize] );

  auto t0 = time_now();
  print( "client-start", timestamp<ms>(), i );
  while ( not node.eof() ) {
    node.read( buf.get(), bufSize );
  }
  print( "client-end", timestamp<ms>(), i, time_diff<ms>( t0 ) );
}

void run( string port, string backends )
{
  TCPSocket sock{IPV4};
  sock.set_reuseaddr();
  sock.set_nodelay();
  sock.set_send_buffer( Knobs4::NET_SND_BUF );
  sock.set_recv_buffer( Knobs4::NET_RCV_BUF );
  sock.bind( { "0.0.0.0", port } );
  sock.listen();

  // wait for all backends to connect
  vector<thread> nodes;
  size_t n = atoll( backends.c_str() );
  for ( size_t i = 0; i < n; i++ ) {
    nodes.emplace_back( runNode, sock.accept(), i );
  }

  // wait for them to finish
  for ( auto & node : nodes ) {
    node.join();
  }
}

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc < 3 ) {
    throw runtime_error( "Usage: " + string( argv[0] )
      + " [port] [backends*disks]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    run( argv[1], argv[2] );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
