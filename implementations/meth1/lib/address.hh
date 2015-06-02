#ifndef ADDRESS_HH
#define ADDRESS_HH

#include <string>
#include <utility>

#include <netinet/in.h>
#include <netdb.h>

/* IP Vesion a socket should use */
enum IPVersion { IPV4 = AF_INET, IPV6 = AF_INET6 };

/* Address class for IPv4/IPv6 addresses */
class Address
{
public:
  typedef union {
    sockaddr as_sockaddr;
    sockaddr_storage as_sockaddr_storage;
  } raw;

private:
  socklen_t size_;

  raw addr_;

  /* private constructor given ip/host, service/port, and optional hints */
  Address( const std::string & node, const std::string & service,
           const addrinfo * hints );

public:
  /* constructors */
  Address();
  Address( const IPVersion ipv );
  Address( const raw & addr, const size_t size );
  Address( const sockaddr & addr, const size_t size );

  /* construct by resolving host name and service name */
  Address( const std::string & hostname, const std::string & service );

  /* construct with numerical IP address and numeral port number */
  Address( const std::string & ip, const uint16_t port );

  /* construct from an IP string with port -- i.e., "192.168.0.3:800" */
  Address( std::string ip );
  Address( const char * ip ) : Address{ std::string( ip ) } {};

  /* accessors */
  sa_family_t domain( void ) const { return addr_.as_sockaddr.sa_family; }
  std::pair<std::string, uint16_t> ip_port( void ) const;
  std::string ip( void ) const { return ip_port().first; }
  uint16_t port( void ) const { return ip_port().second; }
  std::string to_string( void ) const;

  socklen_t size( void ) const { return size_; }
  const sockaddr & to_sockaddr( void ) const;

  const in_addr & to_inaddr( void ) const;
  const in6_addr & to_in6addr( void ) const;

  /* equality */
  bool operator==( const Address & other ) const;
};

#endif /* ADDRESS_HH */
