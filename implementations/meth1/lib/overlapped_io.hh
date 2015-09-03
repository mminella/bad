#ifndef OVERLAPPED_IO_HH
#define OVERLAPPED_IO_HH

#include <cstdlib>
#include <iostream>
#include <memory>
#include <system_error>
#include <thread>

#include "tune_knobs.hh"

#include "circular_io.hh"
#include "channel.hh"
#include "file.hh"
#include "timestamp.hh"

/**
 * Specializes CircularIO to deal with Files.
 */
class OverlappedIO : public CircularIO
{
private:
  File & file_;
  uint64_t fsize_;

public:
  OverlappedIO( File & file )
    : CircularIO( file )
    , file_{file}
    , fsize_{file.size()}
  {}

  /* no copy or move */
  OverlappedIO( const OverlappedIO & ) = delete;
  OverlappedIO & operator=( const OverlappedIO & ) = delete;
  OverlappedIO( OverlappedIO && ) = delete;
  OverlappedIO & operator=( OverlappedIO && ) = delete;

  void rewind( void )
  {
    file_.rewind();
    start_read( fsize_ );
  }
};

#endif /* OVERLAPPED_IO_HH */
