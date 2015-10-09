#ifndef DOMAIN_SOCKET_HH
#define DOMAIN_SOCKET_HH

#include <utility>

#include "file_descriptor.hh"

/* Unix domain sockets (IPC) */
class UnixDomainSocket : public FileDescriptor
{
private:
  explicit UnixDomainSocket( int fd ) noexcept
    : FileDescriptor( fd )
  {
  }

public:
  /* construct a unix domain socket pair */
  static std::pair<UnixDomainSocket, UnixDomainSocket> NewPair( void );

  /* send a file descriptor over a unix domain socket */
  void send_fd( const FileDescriptor & fd );

  /* receive a file descriptor over a unix domain socket */
  FileDescriptor recv_fd( void );
};

#endif /* DOMAIN_SOCKET_HH */
