#include <memory>
#include <thread>
#include <vector>

#include "address.hh"
#include "exception.hh"
#include "socket.hh"
#include "sync_print.hh"

#include "config_file.hh"
#include "meth4_client.hh"
#include "meth4_knobs.hh"

using namespace std;

void runClient( TCPSocket client )
{
  constexpr size_t bufSize = 1024 * 1024 * 2;
  unique_ptr<char> buf( new char[bufSize] );

  while ( true ) {
    client.read( buf.get(), bufSize );
    if ( client.eof() ) {
      return;
    }
  }
}

void run( string port )
{
  TCPSocket sock{IPV4};
  sock.set_reuseaddr();
  sock.set_nodelay();
  sock.set_send_buffer( Knobs4::NET_SND_BUF );
  sock.set_recv_buffer( Knobs4::NET_RCV_BUF );
  sock.bind( { "0.0.0.0", port } );
  sock.listen();

  size_t i = 0;
  while ( true ) {
    thread t( runClient, sock.accept() );
    t.detach();
    print( "client", i++ );
  }
}

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc < 2 ) {
    // pass in hostname rather than retrieve to allow easy testing
    throw runtime_error( "Usage: " + string( argv[0] ) + " [port]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    run( argv[1] );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
