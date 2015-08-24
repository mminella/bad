#ifndef PIPE_HH
#define PIPE_HH

#include <utility>

#include "file_descriptor.hh"

/* class for unix pipes */
class Pipe : public FileDescriptor
{
public:
  enum class Side { Read, Write };

private:
  const Side side_;

protected:
  Pipe( int fd, Side side ) noexcept;

public:
  /* construct a pipe, returning the two end-points */
  static std::pair<Pipe, Pipe> NewPair( void );

  /* which side of the pipe do we have? */
  const Side & side( void ) const { return side_; }
};

#endif /* PIPE_HH */
