/* Straight-forward (write/read) network bandwidth test.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

constexpr size_t TRANSFER = size_t( 1024 ) * 1024 * 1024 * 20;
constexpr size_t BLKSIZE = size_t( 1024 ) * 1024 * 10;
constexpr unsigned long long NS = 1000000000;
constexpr size_t MB = size_t( 1024 ) * 1024;
constexpr size_t TCPBUF = size_t( 1024 ) * 1024 * 2;

int main( int argc, char * argv[] )
{
	int cfd, optval;
  struct addrinfo * addr;
  struct addrinfo hints;
  size_t sent;
  ssize_t n;
  struct timespec ts;
  char * buf;
  unsigned long long nsec1, nsec2;

  if ( argc != 3 ) {
    fprintf( stderr, "Usage: %s [host] [port]\n", argv[0] );
    return EXIT_FAILURE;
  }

  // create socket
	if ( ( cfd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
    perror( "Error creating client socket" );
    return EXIT_FAILURE;
  }

  // set some useful options
  optval = 1;
	setsockopt( cfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval) );
	setsockopt( cfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval) );
	setsockopt( cfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval) );

  optval = TCPBUF;
  setsockopt( cfd, SOL_SOCKET, SO_RCVBUF, &optval, sizeof( optval ) );
  setsockopt( cfd, SOL_SOCKET, SO_SNDBUF, &optval, sizeof( optval ) );
	// setsockopt( cfd, IPPROTO_TCP, TCP_MAXSEG, &optval, sizeof(optval) );

  // resolve address
  memset( &hints, 0, sizeof( addrinfo ) );
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if ( getaddrinfo( argv[1], argv[2], &hints, &addr ) != 0 ) {
    perror( "Error resolving address" );
    return EXIT_FAILURE;
  }

  // connect to server
  if ( connect( cfd, addr->ai_addr, addr->ai_addrlen ) != 0 ) {
    perror( "Error connecting to server" );
    return EXIT_FAILURE;
  }

  // allocate transfer buffer
  if ( ( buf = (char *) malloc( BLKSIZE ) ) == nullptr ) {
    perror( "Error allocating buffer" );
  }

  // send the data
  clock_gettime( CLOCK_MONOTONIC, &ts );
  nsec1 = ts.tv_sec * NS + ts.tv_nsec;
  for ( sent = 0; sent < TRANSFER; ) {
    n = write( cfd, buf, BLKSIZE );
    if ( n < 0 ) {
      perror( "Error sending data" );
    } else if ( n == 0 ) {
      fprintf( stderr, "Sent zero data\n" );
    }
    sent += n;
  }
  clock_gettime( CLOCK_MONOTONIC, &ts );
  nsec2 = ts.tv_sec * NS + ts.tv_nsec;

  // calculate bandwidth
  nsec2 -= nsec1;
  printf( "Transferred: %lu MBs\n", TRANSFER / MB );
  printf( "Time: %.2f s\n", (double)( nsec2 ) / NS );
  printf( "Bandwidth: %llu MB/s\n", TRANSFER / MB / ( nsec2 / NS ) );

  // free dynamic memory
  freeaddrinfo( addr );
  free( buf );

  return EXIT_SUCCESS;
}

