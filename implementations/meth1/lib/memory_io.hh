#ifndef MEMORY_IO_HH
#define MEMORY_IO_HH

#include <cstring>

#include "file.hh"
#include "circular_io.hh"

/**
 * Reads file into memory (in parallel with serving that data to clients), and
 * caches the file so that all future reads are from memory.
 */
template<size_t rec_size>
class MemoryIO
{
private:
  CircularIO io_;
  char * bstart_;
  char * bend_;
  char * bready_;
  char * pos_;

public:
  MemoryIO( File & file )
    : io_{file, 10, (uint64_t) file.fd_num()}
    , bstart_{new char[file.size()]}
    , bend_{bstart_ + file.size()}
    , bready_{bstart_}
    , pos_{bstart_}
  {
    file.rewind();
  };

  /* no copy or move */
  MemoryIO( const MemoryIO & r ) = delete;
  MemoryIO( MemoryIO && r ) = delete;
  MemoryIO & operator=( const MemoryIO & r ) = delete;
  MemoryIO & operator=( MemoryIO && r ) = delete;

  void rewind( void ) noexcept { pos_ = bstart_; }

  void load( void ) noexcept
  {
    rewind();
    while( true ) {
      auto r = next_record();
      if ( r == nullptr ) {
        break;
      }
    }
  }

  const char * next_record( void ) noexcept
  {
    while ( true ) {
      if ( pos_ + rec_size <= bready_ ) {
        const char * p = pos_;
        pos_ += rec_size;
        return p;
      } else if ( pos_ + rec_size > bend_ ) {
        return nullptr;
      } else {
        auto blk = io_.next_block();
        if ( blk.first == nullptr ) {
          return nullptr;
        }
        memcpy( pos_, blk.first, blk.second );
        bready_ += blk.second;
      }
    }
  }
};

#endif /* MEMORY_IO_HH */
