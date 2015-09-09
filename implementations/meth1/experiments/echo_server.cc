/**
 * Simple test TCP server for playing with libbasicrts.
 */
#include <iostream>
#include <thread>

#include "buffered_io.hh"
#include "socket.hh"

using namespace std;

int main( int argc, char *argv[] )
{
  if ( argc != 2 ) {
    cerr << "Usage: " << argv[ 0 ] << " PORT" << endl;
    return EXIT_FAILURE;
  }

  TCPSocket sock;
  sock.set_reuseaddr();
  sock.bind( Address( "::0", argv[ 1 ] ) );

  sock.listen();
  cerr << "Listening on local address: "
       << sock.local_address().to_string() << endl;

  while ( true ) {
    thread client_thread( [] ( TCPSocket client ) {
      BufferedIO_O<TCPSocket> bio { move( client ) };
      cerr << "New connection from "
           << bio.io().peer_address().to_string() << endl;
      while ( true ) {
        const string chunk = bio.IODevice::read();
        if ( bio.eof() ) { break; }
        cerr << "Got " << chunk.size() << " bytes from "
             << bio.io().peer_address().to_string() << ": " << chunk;
        bio.IODevice::write( chunk );
        bio.flush( true );
      }

      cerr << bio.io().peer_address().to_string()
           << " closed the connection." << endl; 
    }, sock.accept() );

    client_thread.detach();
  }

  return EXIT_SUCCESS;
}

