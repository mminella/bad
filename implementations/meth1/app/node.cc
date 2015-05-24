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
         << " - Time: " << dg.timestamp << endl << " ---------------" << endl;

    const uint8_t * const buf =
      reinterpret_cast<const uint8_t * const>( dg.payload.data() );

    print_ip_packet( buf, dg.payload.length() );

    uint8_t ipv = ( buf[0] & 0xf0 ) >> 4;
    if ( ipv != 4 ) {
      cerr << "ERR: unsupported IP version" << endl;
      continue;
    }

    uint8_t * mbuf = const_cast<uint8_t *>( buf );

    // store new IP
    // inet_aton( "10.0.1.93", (in_addr *)( mbuf + 12 ) );

    // swap IP's
    in_addr * src = reinterpret_cast<in_addr *>( mbuf + 12 );
    in_addr * dst = reinterpret_cast<in_addr *>( mbuf + 16 );
    in_addr addr = *src;
    *src = *dst;
    *dst = addr;

    // zero out old checksum
    mbuf[10] = mbuf[11] = 0;

    // calculate new checksum
    size_t count = ( buf[0] & 0x0f ) * 4;
    uint16_t chk = internet_checksum( buf, count );
    mbuf[10] = chk, mbuf[11] = chk >> 8;

    // swap ports if UDP or TCP
    if ( buf[9] == 17 || buf[9] == 6 ) {
      uint16_t * sport = reinterpret_cast<uint16_t *>( mbuf + count );
      uint16_t * dport = reinterpret_cast<uint16_t *>( mbuf + count + 2 );
      uint16_t port = *sport;
      *sport = *dport;
      *dport = port;
    }

    cerr << " ----" << endl;

    print_ip_packet( buf, dg.payload.length() );

    cerr << " ---------------" << endl;

    // send back the packet to client
    sock.sendto( dg.source_address, dg.payload );
  }

  return EXIT_SUCCESS;
}
