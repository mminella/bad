#ifndef DOMAIN_SOCKET_HH
#define DOMAIN_SOCKET_HH

#include "file_descriptor.hh"

class UnixDomainSocket : public FileDescriptor
{
private:
  UnixDomainSocket( const int s_fd )
    : FileDescriptor( s_fd )
  {
  }

public:
  static std::pair<UnixDomainSocket, UnixDomainSocket> NewPair( void );

  void send_fd( FileDescriptor & fd );
  FileDescriptor recv_fd( void );
};

#endif /* DOMAIN_SOCKET_HH */
