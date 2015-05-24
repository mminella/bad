#include <net/route.h>
#include <sys/ioctl.h>

#include "address.hh"
#include "exception.hh"
#include "route.hh"
#include "socket.hh"
#include "util.hh"

/* create default route */
void create_default_route( const Address & addr )
{
  rtentry route;
  zero( route );

  route.rt_gateway = addr.to_sockaddr();
  route.rt_dst = route.rt_genmask = Address{IPV4}.to_sockaddr();
  route.rt_flags = RTF_UP | RTF_GATEWAY;

  SystemCall( "ioctl SIOCADDRT",
              ioctl( UDPSocket{IPV4}.fd_num(), SIOCADDRT, &route ) );
}
