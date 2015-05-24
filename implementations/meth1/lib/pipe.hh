#ifndef PIPE_HH
#define PIPE_HH

#include "address.hh"
#include "file_descriptor.hh"

/* class for unix pipes */
class Pipe : public FileDescriptor
{
public:
  enum class Side { Read, Write };

private:
  Side side_;

protected:
  Pipe( const int fd, const Side side );

public:
  /* static constructor for a pipe pair */
  static std::pair<Pipe, Pipe> NewPair( void );

  /* forbid copying */
  Pipe( const Pipe & other ) = delete;
  const Pipe & operator=( const Pipe & other ) = delete;

  /* allow move constructor */
  Pipe( Pipe && other );

  const Side & side( void ) const { return side_; }
};

#endif /* PIPE_HH */
