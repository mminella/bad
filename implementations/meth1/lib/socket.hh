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
  /* constructor */
  Socket( int domain, int type, int protocol = 0 );

  /* construct from file descriptor */
  Socket( FileDescriptor && s_fd, int domain, int type );

  /* forbid copying or assignment */
  Socket( const Socket & other ) = delete;
  Socket & operator=( const Socket & other ) = delete;

  /* allow moving */
  Socket( Socket && other ) noexcept;
  Socket & operator=( Socket && other ) noexcept;

  /* destructor */
  virtual ~Socket() = default;

  /* set socket option */
  template <typename option_type>
  void setsockopt( int level, int option, const option_type & option_value );

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
  struct received_datagram {
    Address source_address;
    uint64_t timestamp;
    std::string payload;
  };

  /* default constructor, creates a IPV6 UDP Socket. */
  UDPSocket()
    : Socket( IPV6, SOCK_DGRAM )
  {
  }

  /* constructor, creates UDP socket of specified IP version. */
  UDPSocket( IPVersion ipv )
    : Socket( ipv, SOCK_DGRAM )
  {
  }

  /* forbid copying or assignment */
  UDPSocket( const UDPSocket & other ) = delete;
  UDPSocket & operator=( const UDPSocket & other ) = delete;

  /* allow moving */
  UDPSocket( UDPSocket && other ) = default;
  UDPSocket & operator=( UDPSocket && other ) = default;

  /* destructor */
  ~UDPSocket() = default;

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
  TCPSocket( int domain, FileDescriptor && fd )
    : Socket( std::move( fd ), domain, SOCK_STREAM )
  {
  }

public:
  /* default constructor, creates a IPV6 TCP Socket. */
  TCPSocket()
    : Socket( IPV6, SOCK_STREAM )
  {
  }

  /* constructor, creates TCP socket of specified IP version. */
  TCPSocket( IPVersion ipv )
    : Socket( ipv, SOCK_STREAM )
  {
  }

  /* forbid copying or assignment */
  TCPSocket( const TCPSocket & other ) = delete;
  TCPSocket & operator=( const TCPSocket & other ) = delete;

  /* allow moving */
  TCPSocket( TCPSocket && other ) = default;
  TCPSocket & operator=( TCPSocket && other ) = default;

  /* destructor */
  ~TCPSocket() = default;

  /* mark the socket as listening for incoming connections */
  void listen( int backlog = 16 );

  /* accept a new incoming connection */
  TCPSocket accept( void );
};

/* RAW socket */
class RAWSocket : public Socket
{
public:
  /* default constructor, creates a IPV6 RAW Socket. */
  RAWSocket()
    : RAWSocket( IPV6 ){};

  /* constructor, creates RAW socket of specified IP version. */
  RAWSocket( IPVersion ipv );

  /* forbid copying or assignment */
  RAWSocket( const RAWSocket & other ) = delete;
  RAWSocket & operator=( const RAWSocket & other ) = delete;

  /* allow moving */
  RAWSocket( RAWSocket && other ) = default;
  RAWSocket & operator=( RAWSocket && other ) = default;

  /* destructor */
  ~RAWSocket() = default;

  /* send datagram to specified address */
  void sendto( const Address & peer, const std::string & payload );

  /* send datagram to connected address */
  void send( const std::string & payload );
};

#endif /* SOCKET_HH */
