#include <iostream>
#include <thread>

#include <arpa/inet.h>
#include <linux/if_tun.h>

#include "exception.hh"
#include "ip.hh"
#include "netdevice.hh"
#include "pipe.hh"
#include "poller.hh"
#include "socket.hh"
#include "system_runner.hh"
#include "util.hh"

using namespace std;
using namespace PollerShortNames;

int run( int argc, char * argv[] );

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc != 2 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) + " [port]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    run( argc, argv );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int run( int argc, char * argv[] )
{
  sanity_check_env( argc );
  check_usage( argc, argv );

  string port{argv[1]};

  UDPSocket sock;
  sock.set_reuseaddr();
  sock.bind( Address( "::0", port ) );

  cerr << "Listening on local address: " << sock.local_address().to_string()
       << endl;

  while ( true ) {
    UDPSocket::received_datagram dg = sock.recv();

    if ( sock.eof() ) {
      break;
    }

    cerr << "Received datagram from: " << dg.source_address.to_string() << endl
         << " - Time: " << dg.timestamp << endl
         << " ---------------" << endl;

    // send back the packet to client
    sock.sendto( dg.source_address, dg.payload );
  }

  return EXIT_SUCCESS;
}
