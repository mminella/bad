#ifndef CIRCULAR_IO_HH
#define CIRCULAR_IO_HH

#include <cstdlib>
#include <iostream>
#include <memory>
#include <system_error>
#include <thread>

#include "tune_knobs.hh"

#include "channel.hh"
#include "file.hh"
#include "sync_print.hh"
#include "timestamp.hh"

/**
 * Read from an IODevice in a seperate thread in a block-wise fashion, using a
 * channel for notifying when new blocks are ready.
 */
class CircularIO
{
public:
  static constexpr size_t BLOCK = Knobs::IO_BLOCK;
  static constexpr size_t ALIGNMENT = IODevice::ODIRECT_ALIGN;

  using block_ptr = std::pair<const char *, size_t>;

private:
  IODevice & io_;
  char * buf_;
  size_t bufSize_;
  Channel<block_ptr> blocks_;
  Channel<size_t> start_;
  std::thread reader_;
  std::function<void(void)> io_cb_;

  /* debug info */
  size_t readPass_;
  int id_;

  /* continually read from the device, taking commands over a channel */
  void read_loop( void );

public:
  CircularIO( IODevice & io, size_t blocks, int id = 0 );

  /* no copy or move */
  CircularIO( const CircularIO & r ) = delete;
  CircularIO & operator=( const CircularIO & r ) = delete;
  CircularIO( CircularIO && r ) = delete;
  CircularIO & operator=( CircularIO && r ) = delete;

  ~CircularIO( void );

  void start_read( size_t nbytes );
  block_ptr next_block( void );
  void set_io_drained_cb( std::function<void(void)> f );

  int id( void ) const noexcept;
};

#endif /* CIRCULAR_IO_HH */
