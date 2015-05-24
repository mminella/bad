#include <cassert>

#include <fcntl.h>
#include <unistd.h>

#include <linux/if_tun.h>
#include <sys/ioctl.h>

#include "address.hh"
#include "exception.hh"
#include "netdevice.hh"
#include "socket.hh"
#include "system_runner.hh"
#include "util.hh"

using namespace std;

/* Apply an ifreq adjustment to the specified interface */
void interface_ioctl( FileDescriptor & fd, const int request,
                      const string & name,
                      function<void( ifreq & ifr )> ifr_adjustment )
{
  ifreq ifr;
  zero( ifr );
  /* the interface name is the only identifier */
  strncpy( ifr.ifr_name, name.c_str(), IFNAMSIZ );

  ifr_adjustment( ifr );

  SystemCall( "ioctl " + name,
              ioctl( fd.fd_num(), request, static_cast<void *>( &ifr ) ) );
}

/* Apply an ifreq adjustment to the specified interface */
void interface_ioctl( FileDescriptor && fd, const int request,
                      const string & name,
                      function<void( ifreq & ifr )> ifr_adjustment )
{
  interface_ioctl( fd, request, name, ifr_adjustment );
}

/* Assign a network address to the interface specified */
void assign_address( const string & device_name, const Address & addr,
                     const Address & peer, const Address & mask )
{
  UDPSocket ioctl_socket{IPV4};

  /* assign address */
  interface_ioctl( ioctl_socket, SIOCSIFADDR, device_name,
                   [&]( ifreq & ifr ) { ifr.ifr_addr = addr.to_sockaddr(); } );

  /* destination */
  interface_ioctl( ioctl_socket, SIOCSIFDSTADDR, device_name,
                   [&]( ifreq & ifr ) { ifr.ifr_addr = peer.to_sockaddr(); } );

  /* mask */
  interface_ioctl(
    ioctl_socket, SIOCSIFNETMASK, device_name,
    [&]( ifreq & ifr ) { ifr.ifr_netmask = mask.to_sockaddr(); } );
}

/* Set interface up state */
void set_if_up_state( const std::string & device_name, bool up )
{
  /* bring up localhost */
  if ( up ) {
    interface_ioctl(
      UDPSocket( IPV4 ), SIOCSIFFLAGS, device_name,
      []( ifreq & ifr ) { ifr.ifr_flags = IFF_UP | IFF_RUNNING; } );
  } else {
    interface_ioctl( UDPSocket( IPV4 ), SIOCSIFFLAGS, device_name,
                     []( ifreq & ifr ) { ifr.ifr_flags = 0; } );
  }
}

/* A TUN device for layer 3 packet manipulation */
TunDevice::TunDevice( const string & name, const Address & addr,
                      const Address & peer, const Address & mask,
                      const bool packet_header )
  : FileDescriptor(
      SystemCall( "open /dev/net/tun", open( "/dev/net/tun", O_RDWR ) ) )
{
  int flags = IFF_TUN | ( packet_header ? 0 : IFF_NO_PI );

  interface_ioctl( *this, TUNSETIFF, name,
                   [flags]( ifreq & ifr ) { ifr.ifr_flags = flags; } );

  assign_address( name, addr, peer, mask );
  set_if_up_state( name, true );
}

// Helper function to construct a MacVTapSocket.
static int openVTapDevice( const string & name, const string & lower_dev,
                           const Address & addr, const Address & mask,
                           const string & mac )
{
  run( {IP, "link", "add", "link", lower_dev, "name", name, "address", mac,
        "type", "macvtap"} );

  string path = "/sys/class/net/" + name + "/ifindex";
  FileDescriptor fd1{
    SystemCall( "open " + path, open( path.c_str(), O_RDONLY ) )};
  string ifindex = fd1.read();
  ifindex.pop_back(); // pop '\n'
  path = "/dev/tap" + ifindex;

  UDPSocket ioctl_socket{IPV4};
  interface_ioctl( ioctl_socket, SIOCSIFADDR, name,
                   [&]( ifreq & ifr ) { ifr.ifr_addr = addr.to_sockaddr(); } );
  interface_ioctl( ioctl_socket, SIOCSIFNETMASK, name, [&]( ifreq & ifr ) {
    ifr.ifr_netmask = mask.to_sockaddr();
  } );

  // open tap device
  return SystemCall( "open", open( path.c_str(), O_RDONLY ) );
}

/* A MacVTap device for programmable virtual nic's from real nic's */
MacVTapSocket::MacVTapSocket( const string & name, const string & lower_dev,
                              const Address & addr, const Address & mask,
                              const string & mac )
  : IODevice{openVTapDevice( name, lower_dev, addr, mask, mac ), -1}
  , raw_{IPV4}
  , name_{name}
  , addr_{addr}
  , persistent_{false}
  , only_ip_{true}
  , filter_ip_{true}
{
  fd_w_ = raw_.fd_num();
}

/* Destructor */
MacVTapSocket::~MacVTapSocket()
{
  if ( fd_r_ >= 0 ) {
    try {
      SystemCall( "close", close( fd_r_ ) );
    } catch ( const exception & e ) { /* don't throw from destructor */
      print_exception( e );
    }
  }

  // while, RAWSocket will close raw_

  if ( persistent_ ) {
    return;
  }

  try {
    run( {IP, "link", "del", name_} );
  } catch ( const std::exception & e ) {
    print_exception( e );
  }
}

/* Set the route outgoing packets should take */
void MacVTapSocket::set_route( const Address & route )
{
  raw_.connect( route );
}

/* read method */
string MacVTapSocket::read( const size_t limit )
{
  char buffer[BUFFER_SIZE], *buf;
  ssize_t bytes_read;
  uint32_t ip =
    ( (struct sockaddr_in *)&addr_.to_sockaddr() )->sin_addr.s_addr;
  uint16_t ethtype;

  assert( limit > 0 );

  bytes_read = SystemCall(
    "read", ::read( fd_r_, buffer, min( sizeof( buffer ), limit ) ) );

  register_read();

  if ( bytes_read == 0 ) {
    set_eof();
    return string( buffer, bytes_read );
  } else {
    // no fucking idea what these first 10 bytes are, can't find documented
    buf = buffer + 10;

    // filter out non-IP, and not addressed to us.
    ethtype = ntohs( *(uint16_t *)( buf + 12 ) );
    if ( ( only_ip_ and ethtype != ETH_P_IP ) or
         ( filter_ip_ and *(in_addr_t *)( buf + ETH_HLEN + 16 ) != ip ) ) {
      return "";
    }

    return string( buf, bytes_read - 10 );
  }
}
