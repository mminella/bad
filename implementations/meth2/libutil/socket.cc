#include <netinet/tcp.h>
#include <sys/socket.h>

#include "socket.hh"
#include "exception.hh"
#include "timestamp.hh"
#include "util.hh"

using namespace std;

/* full constructor */
Socket::Socket( int domain, int type, int protocol )
  : FileDescriptor( SystemCall( "socket", socket( domain, type, protocol ) ) )
{
}

/* construct from file descriptor */
Socket::Socket( FileDescriptor && fd, int domain, int type )
  : FileDescriptor( move( fd ) )
{
  int actual_value;
  socklen_t len;

  /* verify domain */
#ifdef linux
  len = sizeof( actual_value );
  SystemCall( "getsockopt", getsockopt( fd_num(), SOL_SOCKET, SO_DOMAIN,
                                        &actual_value, &len ) );
#else
  actual_value = local_address().domain();
  len = sizeof( actual_value );
#endif
  if ( ( len != sizeof( actual_value ) ) or ( actual_value != domain ) ) {
    throw runtime_error( "socket domain mismatch" );
  }

  /* verify type */
  len = sizeof( actual_value );
  SystemCall( "getsockopt", getsockopt( fd_num(), SOL_SOCKET, SO_TYPE,
                                        &actual_value, &len ) );
  if ( ( len != sizeof( actual_value ) ) or ( actual_value != type ) ) {
    throw runtime_error( "socket type mismatch" );
  }
}

/* set socket option */
template <typename option_t>
void Socket::setsockopt( const int level, const int option,
                         const option_t & option_value )
{
  SystemCall( "setsockopt",
              ::setsockopt( fd_num(), level, option, &option_value,
                            sizeof( option_value ) ) );
}

/* get the local address the socket is connected to */
Address Socket::local_address( void ) const
{
  Address::raw addr;
  socklen_t n = sizeof( addr );
  SystemCall( "getsockname", getsockname( fd_num(), &addr.as_sockaddr, &n ) );
  return {addr, n};
}

/* allow local address to be reused sooner, at the cost of some robustness */
void Socket::set_reuseaddr( void )
{
  setsockopt( SOL_SOCKET, SO_REUSEADDR, int( true ) );
}

/* enable underlying protocol sending periodic keep-alive messages */
void Socket::set_keepalive( void )
{
  setsockopt( SOL_SOCKET, SO_KEEPALIVE, int( true ) );
}

/* set the kernel side send buffer size */
void Socket::set_send_buffer( size_t size )
{
  setsockopt( SOL_SOCKET, SO_SNDBUF, size );
}

/* set the kernel side receive buffer size */
void Socket::set_recv_buffer( size_t size )
{
  setsockopt( SOL_SOCKET, SO_RCVBUF, size );
}

void Socket::set_nosigpipe( void )
{
#ifdef SO_NOSIGPIPE
  setsockopt( SOL_SOCKET, SO_NOSIGPIPE, int( true ) );
#endif
}

/* get the peer address the socket is connected to */
Address Socket::peer_address( void ) const
{
  Address::raw addr;
  socklen_t n = sizeof( addr );
  SystemCall( "getpeername", getpeername( fd_num(), &addr.as_sockaddr, &n ) );
  return {addr, n};
}

/* bind socket to a specified local address (usually to listen/accept) */
void Socket::bind( const Address & addr )
{
  SystemCall( "bind", ::bind( fd_num(), &addr.to_sockaddr(), addr.size() ) );
}

/* connect socket to a specified peer address */
void Socket::connect( const Address & addr )
{
  SystemCall(
    "connect", ::connect( fd_num(), &addr.to_sockaddr(), addr.size() ) );
}

/* receive datagram and where it came from */
UDPSocket::received_datagram UDPSocket::recv( void )
{
  static const size_t RECEIVE_MTU = 65536;

  /* receive source address, timestamp and payload */
  Address::raw datagram_source_address;
  msghdr header;
  zero( header );
  iovec msg_iovec;
  zero( msg_iovec );

  char msg_payload[RECEIVE_MTU];
  char msg_control[RECEIVE_MTU];

  /* prepare to get the source address */
  header.msg_name = &datagram_source_address;
  header.msg_namelen = sizeof( datagram_source_address );

  /* prepare to get the payload */
  msg_iovec.iov_base = msg_payload;
  msg_iovec.iov_len = sizeof( msg_payload );
  header.msg_iov = &msg_iovec;
  header.msg_iovlen = 1;

  /* prepare to get the timestamp */
  header.msg_control = msg_control;
  header.msg_controllen = sizeof( msg_control );

  /* call recvmsg */
  ssize_t recv_len = SystemCall( "recvmsg", recvmsg( fd_num(), &header, 0 ) );

  register_read();

  /* make sure we got the whole datagram */
  if ( header.msg_flags & MSG_TRUNC ) {
    throw runtime_error( "recvfrom (oversized datagram)" );
  } else if ( header.msg_flags ) {
    throw runtime_error( "recvfrom (unhandled flag)" );
  }

  uint64_t ts = -1;

/* find the timestamp header (if there is one) */
#if defined( SO_TIMESTAMPNS ) || defined( SO_TIMESTAMP )
  cmsghdr * ts_hdr = CMSG_FIRSTHDR( &header );
  while ( ts_hdr ) {
#if defined( SO_TIMESTAMPNS )
    if ( ts_hdr->cmsg_level == SOL_SOCKET and
         ts_hdr->cmsg_type == SCM_TIMESTAMPNS ) {
      const timespec * const kernel_time =
        reinterpret_cast<timespec *>( CMSG_DATA( ts_hdr ) );
      ts = timestamp<ms>( *kernel_time );
    }
#elif defined( SO_TIMESTAMP )
    if ( ts_hdr->cmsg_level == SOL_SOCKET and
         ts_hdr->cmsg_type == SCM_TIMESTAMP ) {
      const timeval * const kernel_time =
        reinterpret_cast<timeval *>( CMSG_DATA( ts_hdr ) );
      ts = timestamp<ms>( *kernel_time );
    }
#endif
    ts_hdr = CMSG_NXTHDR( &header, ts_hdr );
  }
#endif

  received_datagram ret = {
    Address( datagram_source_address, header.msg_namelen ), ts,
    string( msg_payload, recv_len )};

  return ret;
}

/* send datagram to specified address */
void UDPSocket::sendto( const Address & destination, const string & payload )
{
  const ssize_t bytes_sent = SystemCall(
    "sendto", ::sendto( fd_num(), payload.data(), payload.size(), 0,
                        &destination.to_sockaddr(), destination.size() ) );

  register_write();

  if ( size_t( bytes_sent ) != payload.size() ) {
    throw runtime_error( "datagram payload too big for sendto()" );
  }
}

/* send datagram to connected address */
void UDPSocket::send( const string & payload )
{
  const ssize_t bytes_sent = SystemCall(
    "send", ::send( fd_num(), payload.data(), payload.size(), 0 ) );

  register_write();

  if ( size_t( bytes_sent ) != payload.size() ) {
    throw runtime_error( "datagram payload too big for send()" );
  }
}

/* turn on timestamps on receipt */
void UDPSocket::set_timestamps( void )
{
#if defined( SO_TIMESTAMPNS )
  setsockopt( SOL_SOCKET, SO_TIMESTAMPNS, int( true ) );
#elif defined( SO_TIMESTAMP )
  setsockopt( SOL_SOCKET, SO_TIMESTAMP, int( true ) );
#endif
}

void TCPSocket::set_nodelay( void )
{
  setsockopt( IPPROTO_TCP, TCP_NODELAY, int( true ) );
}

/* mark the socket as listening for incoming connections */
void TCPSocket::listen( const int backlog )
{
  SystemCall( "listen", ::listen( fd_num(), backlog ) );
}

/* accept a new incoming connection */
TCPSocket TCPSocket::accept( void )
{
  Address addr = local_address();
  int ipv = addr.domain();
  int fd = SystemCall( "accept", ::accept( fd_num(), nullptr, nullptr ) );
  register_read();
  return {{fd}, ipv};
}
