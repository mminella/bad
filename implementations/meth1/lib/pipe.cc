#include "pipe.hh"
#include "exception.hh"

#include <unistd.h>

using namespace std;

/* construct from fd number */
Pipe::Pipe( const int fd, const Side side )
  : FileDescriptor{fd}
  , side_{side}
{
}

/* move constructor */
Pipe::Pipe( Pipe && other )
  : FileDescriptor{move( other )}
  , side_{other.side_}
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
