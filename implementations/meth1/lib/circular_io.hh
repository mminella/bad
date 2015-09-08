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
  static constexpr uint64_t BLOCK = Knobs::IO_BLOCK;
  static constexpr uint64_t ALIGNMENT = 4096;

  using block_ptr = std::pair<const char *, uint64_t>;

private:
  IODevice & io_;
  char * buf_;
  uint64_t bufSize_;
  Channel<block_ptr> blocks_;
  Channel<uint64_t> start_;
  std::thread reader_;
  uint64_t readPass_;
  std::function<void(void)> io_cb_;

  /* continually read from the device, taking commands over a channel */
  void read_loop( void );

protected:
  uint64_t id_;

public:
  CircularIO( IODevice & io, uint64_t blocks, uint64_t id = 0 );
  /* no copy */
  CircularIO( const CircularIO & r ) = delete;
  CircularIO & operator=( const CircularIO & r ) = delete;

  /* no move */
  CircularIO( CircularIO && r ) = delete;
  CircularIO & operator=( CircularIO && r ) = delete;

  ~CircularIO( void );

  void start_read( uint64_t nbytes );
  block_ptr next_block( void );
  void set_io_drained_cb( std::function<void(void)> f );
};

#endif /* CIRCULAR_IO_HH */
