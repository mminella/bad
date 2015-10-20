/* Straight-forward (write/read) network bandwidth test.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <string>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

constexpr int BACKLOG = 128;
constexpr size_t BLKSIZE = size_t( 1024 ) * 1024 * 10;
constexpr size_t TCPBUF = size_t( 1024 ) * 1024 * 2;

int main( int argc, char * argv[] )
{
  int sfd, cfd, optval;
  struct sockaddr_in addr;
  char * buf = (char *) malloc( BLKSIZE );
  size_t cid;
  ssize_t n;
  socklen_t optlen;

  if ( argc != 2 ) {
    fprintf( stderr, "Usage: %s [port]\n", argv[0] );
    return EXIT_FAILURE;
  }

  // create socket
  if ( ( sfd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
    perror( "Error creating server socket" );
    return EXIT_FAILURE;
  }

  // set some useful options
  optval = 1;
  if ( setsockopt( sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval) ) < 0 ) {
    perror( "Error setting SO_REUSEADDR" );
  }
  if ( setsockopt( sfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval) ) < 0 ) {
    perror( "Error setting SO_KEEPALIVE" );
  }
  if ( setsockopt( sfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval) ) < 0 ) {
    perror( "Error setting TCP_NODELAY" );
  }

  optval = TCPBUF;
  if ( setsockopt( sfd, SOL_SOCKET, SO_RCVBUF, &optval, sizeof( optval ) ) < 0 ) {
    perror( "Error setting SO_RCVBUF" );
  }
  if ( setsockopt( sfd, SOL_SOCKET, SO_SNDBUF, &optval, sizeof( optval ) ) < 0 ) {
    perror( "Error setting SO_SNDBUF" );
  }
  // setsockopt( cfd, IPPROTO_TCP, TCP_MAXSEG, &optval, sizeof(optval) );

  // validate server socket options are set
  getsockopt( sfd, SOL_SOCKET, SO_RCVBUF, &optval, &optlen );
  if ( optval != TCPBUF * 2 ) {
    fprintf( stderr, "server socket receive buffer size not set: %d\n", optval );
  }
  getsockopt( sfd, SOL_SOCKET, SO_SNDBUF, &optval, &optlen );
  if ( optval != TCPBUF * 2 ) {
    fprintf( stderr, "server socket send buffer size not set: %d\n", optval );
  }
  getsockopt( sfd, IPPROTO_TCP, TCP_NODELAY, &optval, &optlen );
  if ( optval != 1 ) {
    fprintf( stderr, "server TCP no-delay not set\n" );
  }

  // set address to 0.0.0.0:9000
  memset(&addr, 0, sizeof( struct sockaddr_in ) );
  addr.sin_family = AF_INET;
  addr.sin_port = htons( std::stoi( argv[1] ) );
  addr.sin_addr.s_addr = htonl( INADDR_ANY );

  if ( bind( sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in) ) ) {
    perror( "Error binding address to server fd" );
    return EXIT_FAILURE;
  }

  if ( listen( sfd, BACKLOG ) ) {
    perror( "Error setting server fd into listen mode" );
    return EXIT_FAILURE;
  }

  cid = 0;
  while ( true ) {
    cid++;

    if ( ( cfd = accept( sfd, nullptr, nullptr ) ) < 0 ) {
      perror( "Error accepting new client connection" );
      return EXIT_FAILURE;
    }
    printf( "New client [%lu]...\n", cid );

    // check if socket options inherited from listening socket
    getsockopt( cfd, SOL_SOCKET, SO_RCVBUF, &optval, &optlen );
    if ( optval != TCPBUF * 2 ) {
      fprintf( stderr, "socket receive buffer size not inherited: %d\n", optval );
    }
    getsockopt( cfd, SOL_SOCKET, SO_SNDBUF, &optval, &optlen );
    if ( optval != TCPBUF * 2 ) {
      fprintf( stderr, "socket send buffer size not inherited: %d\n", optval );
    }
    getsockopt( cfd, IPPROTO_TCP, TCP_NODELAY, &optval, &optlen );
    if ( optval != 1 ) {
      fprintf( stderr, "TCP no-delay not inherited\n" );
    }

    while ( true ) {
      if ( (n = read( cfd, buf, BLKSIZE ) ) < 0 ) {
        perror( "Error reading from client" );
        break;
      } else if ( n == 0 ) {
        printf( "Client [%lu] finished...\n", cid );
        break;
      }
      // printf( "Read %ld bytes from client [%lu]\n", n, cid );
    }
  }
  return EXIT_SUCCESS;
}
