#ifndef MEMORY_IO_HH
#define MEMORY_IO_HH

#include <cstring>

#include "file.hh"
#include "overlapped_io.hh"

/**
 * Reads whole file into memory (using a seperate thread) and then serves all
 * future reads from memory.
 */
template<size_t rec_size>
class MemoryIO
{
private:
  OverlappedIO io_;
  char * bstart_;
  char * bend_;
  char * bready_;
  char * pos_;
  bool eof_;

public:
  MemoryIO( File & io )
    : io_{io}, bstart_{new char[io.size()]}, bend_{bstart_ + io.size()}
    , bready_{bstart_}, pos_{bstart_} , eof_{false}
  {
    io_.rewind();
  };

  MemoryIO( const MemoryIO & r ) = delete;
  MemoryIO( MemoryIO && r ) = delete;
  MemoryIO & operator=( const MemoryIO & r ) = delete;
  MemoryIO & operator=( MemoryIO && r ) = delete;

  void rewind( void )
  {
    eof_ = false;
    pos_ = bstart_;
  }

  void load( void )
  {
    rewind();
    while( true ) {
      auto r = next_record();
      if ( r == nullptr ) {
        break;
      }
    }
  }

  bool eof( void ) const noexcept { return eof_; }

  const char * next_record( void )
  {
    if ( eof_ ) {
      return nullptr;
    }

    while ( true ) {
      if ( pos_ + rec_size <= bready_ ) {
        const char * p = pos_;
        pos_ += rec_size;
        return p;
      } else if ( pos_ + rec_size > bend_ ) {
        eof_ = true;
        return nullptr;
      } else {
        auto blk = io_.next_block();
        if ( blk.first == nullptr ) {
          eof_ = true;
          return nullptr;
        }
        memcpy( pos_, blk.first, blk.second );
        bready_ += blk.second;
      }
    }
  }
};

#endif /* MEMORY_IO_HH */
