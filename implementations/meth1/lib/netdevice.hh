#ifndef NETDEVICE_HH
#define NETDEVICE_HH

#include <functional>
#include <string>

#include <linux/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

#include "address.hh"
#include "file_descriptor.hh"
#include "socket.hh"

/* Apply an ifreq adjustment to the specified interface */
void interface_ioctl( FileDescriptor & fd, const int request,
                      const std::string & name,
                      std::function<void( ifreq & ifr )> ifr_adjustment );

/* Apply an ifreq adjustment to the specified interface */
void interface_ioctl( FileDescriptor && fd, const int request,
                      const std::string & name,
                      std::function<void( ifreq & ifr )> ifr_adjustment );

/* Assign a network address to the interface specified */
void assign_address( const std::string & device_name, const Address & addr,
                     const Address & peer,
                     const Address & mask = "255.255.255.255" );

/* Set interface up state */
void set_if_up_state( const std::string & device_name, bool up );

/* A TUN device for layer 3 packet manipulation */
class TunDevice : public FileDescriptor
{
public:
  TunDevice( const std::string & name, const Address & addr,
             const Address & peer, const Address & mask = "255.255.255.255",
             const bool packet_header = false );
};

// TODO: Move to configure
#define IP "/sbin/ip"

/* A MacVTap device for programmable virtual nic's from real nic's */
class MacVTapSocket : public IODevice
{
private:
  RAWSocket raw_;
  const std::string name_;
  const Address addr_;
  bool persistent_;
  bool only_ip_;
  bool filter_ip_;

public:
  /* Default constructor */
  MacVTapSocket( const std::string & name, const std::string & lower_dev,
                 const Address & addr,
                 const Address & mask = "255.255.255.255",
                 const std::string & mac = "00:22:33:44:55:66" );

  /* Destructor */
  ~MacVTapSocket();

  /* forbid copying or assignment */
  MacVTapSocket( const MacVTapSocket & other ) = delete;
  const MacVTapSocket & operator=( const MacVTapSocket & other ) = delete;

  /* Set options */
  void set_persistent( bool persist = true ) { persistent_ = persist; };
  void set_only_ip( bool only_ip = true ) { only_ip_ = only_ip; };
  void set_filter_ip( bool filter_ip = true )
  {
    if ( filter_ip ) {
      only_ip_ = true;
    }
    filter_ip_ = filter_ip;
  };

  /* Set the route outgoing packets should take */
  void set_route( const Address & route );

  /* override read method to add filtering */
  virtual std::string read( const size_t limit = BUFFER_SIZE );
};

#endif /* NETDEVICE_HH */
