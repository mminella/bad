#include <unistd.h>

#include <utility>

#include "pipe.hh"
#include "exception.hh"

using namespace std;

/* construct from fd number */
Pipe::Pipe( int fd, Side side ) noexcept
  : FileDescriptor{fd}
  , side_{side}
{
}

/* construct a pipe, returning the two end-points */
std::pair<Pipe, Pipe> Pipe::NewPair( void )
{
  int chn[2];

  SystemCall( "pipe", ::pipe( chn ) );

  Pipe read{chn[0], Side::Read};
  Pipe write{chn[1], Side::Write};

  return make_pair( move( read ), move( write ) );
}
