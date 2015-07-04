#ifndef SOCKET_HH
#define SOCKET_HH

#include <functional>

#include "address.hh"
#include "file_descriptor.hh"

/* class for network sockets (UDP, TCP, etc.) */
class Socket : public FileDescriptor
{
protected:
  /* constructor */
  Socket( int domain, int type, int protocol = 0 );

  /* construct from file descriptor */
  Socket( FileDescriptor && fd, int domain, int type );

  /* set socket option */
  template <typename option_t>
  void setsockopt( int level, int option, const option_t & option_value );

public:
  /* accessors */
  Address local_address( void ) const;
  Address peer_address( void ) const;

  /* allow local address to be reused sooner, at the cost of some robustness */
  void set_reuseaddr( void );

  /* bind socket to a specified local address (usually to listen/accept) */
  void bind( const Address & addr );

  /* connect socket to a specified peer address */
  void connect( const Address & addr );
};

/* UDP socket */
class UDPSocket : public Socket
{
public:
  struct received_datagram {
    Address src_addr;
    uint64_t timestamp;
    std::string payload;
  };

  /* constructor, creates UDP socket of specified IP version. */
  UDPSocket( IPVersion ipv = IPV6 )
    : Socket( ipv, SOCK_DGRAM )
  {
  }

  /* receive datagram, timestamp, and where it came from */
  received_datagram recv( void );

  /* send datagram to specified address */
  void sendto( const Address & peer, const std::string & payload );

  /* send datagram to connected address */
  void send( const std::string & payload );

  /* turn on timestamps on receipt */
  void set_timestamps( void );
};

/* TCP socket */
class TCPSocket : public Socket
{
private:
  /* private constructor used by accept() */
  TCPSocket( FileDescriptor && fd, int domain )
    : Socket( std::move( fd ), domain, SOCK_STREAM )
  {
  }

public:
  /* constructor, creates TCP socket of specified IP version. */
  TCPSocket( IPVersion ipv = IPV6 )
    : Socket( ipv, SOCK_STREAM )
  {
  }

  /* mark the socket as listening for incoming connections */
  void listen( int backlog = 16 );

  /* accept a new incoming connection */
  TCPSocket accept( void );
};

#endif /* SOCKET_HH */
