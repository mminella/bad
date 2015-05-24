#ifndef SIGNALFD_HH
#define SIGNALFD_HH

#include <sys/signalfd.h>
#include <initializer_list>

#include "file_descriptor.hh"

/* wrapper class for Unix signal masks */
class SignalMask
{
private:
  sigset_t mask_;

public:
  SignalMask( const std::initializer_list<int> signals );
  const sigset_t & mask( void ) const { return mask_; }
  void set_as_mask( void ) const;

  static SignalMask current_mask( void );
  bool operator==( const SignalMask & other ) const;
};

/* wrapper class for signal file descriptor */
class SignalFD
{
private:
  FileDescriptor fd_;

public:
  SignalFD( const SignalMask & signals );

  FileDescriptor & fd( void ) { return fd_; }
  signalfd_siginfo read_signal( void ); /* read one signal */
};

#endif /* SIGNALFD_HH */
