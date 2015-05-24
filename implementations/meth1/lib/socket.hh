#ifndef SOCKET_HH
#define SOCKET_HH

#include <functional>

#include "address.hh"
#include "file_descriptor.hh"

/* class for network sockets (UDP, TCP, etc.) */
class Socket : public FileDescriptor
{
private:
  /* get the local or peer address the socket is connected to */
  Address get_address(
    const std::string & name_of_function,
    const std::function<int(int, sockaddr *, socklen_t *)> & function ) const;

protected:
  /* default constructor */
  Socket( const int domain, const int type );

  /* full constructor */
  Socket( const int domain, const int type, const int protocol );

  /* construct from file descriptor */
  Socket( FileDescriptor && s_fd, const int domain, const int type );

  /* forbid copying or assignment */
  Socket( const Socket & other ) = delete;
  const Socket & operator=( const Socket & other ) = delete;

  /* allow moving */
  Socket( Socket && other );

  /* set socket option */
  template <typename option_type>
  void setsockopt( const int level, const int option,
                   const option_type & option_value );

public:
  /* bind socket to a specified local address (usually to listen/accept) */
  void bind( const Address & address );

  /* connect socket to a specified peer address */
  void connect( const Address & address );

  /* accessors */
  Address local_address( void ) const;
  Address peer_address( void ) const;

  /* allow local address to be reused sooner, at the cost of some robustness */
  void set_reuseaddr( void );
};

/* UDP socket */
class UDPSocket : public Socket
{
public:
  UDPSocket()
    : Socket( IPV6, SOCK_DGRAM )
  {
  }
  UDPSocket( const IPVersion ipv )
    : Socket( ipv, SOCK_DGRAM )
  {
  }

  struct received_datagram {
    Address source_address;
    uint64_t timestamp;
    std::string payload;
  };

  /* receive datagram, timestamp, and where it came from */
  received_datagram recv( void );

  /* send datagram to specified address */
  void sendto( const Address & peer, const std::string & payload );

  /* send datagram to specified address */
  void sendto( const Address & destination,
               const std::string::const_iterator & begin,
               const std::string::const_iterator & end );

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
  TCPSocket( FileDescriptor && fd )
    : Socket( std::move( fd ), IPV6, SOCK_STREAM )
  {
  }

public:
  TCPSocket()
    : Socket( IPV6, SOCK_STREAM )
  {
  }
  TCPSocket( const IPVersion ipv )
    : Socket( ipv, SOCK_STREAM )
  {
  }

  /* mark the socket as listening for incoming connections */
  void listen( const int backlog = 16 );

  /* accept a new incoming connection */
  TCPSocket accept( void );
};

/* RAW socket */
class RAWSocket : public Socket
{
public:
  RAWSocket()
    : RAWSocket( IPV6 ){};
  RAWSocket( const IPVersion ipv );

  /* send datagram to specified address */
  void sendto( const Address & peer, const std::string & payload );

  /* send datagram to connected address */
  void send( const std::string & payload );
};

#endif /* SOCKET_HH */
