#ifndef RECORD_LOC_HH
#define RECORD_LOC_HH

#include <cstdint>
#include <iostream>
#include <utility>

#include "record_common.hh"

/**
 * Record key + disk location
 */
class RecordLoc
{
public:
  uint64_t loc_;  // File Offset
  uint32_t host_; // Host
  uint32_t disk_; // Disk
  uint8_t key_[Rec::KEY_LEN];

  RecordLoc( void ) noexcept : loc_{0}, host_{0}, disk_{0} {}

  RecordLoc( const uint8_t * s, uint64_t loc = 0,
	     uint32_t host = 0, uint32_t disk = 0 )
    : loc_{loc},
      host_{host},
      disk_{disk}
  {
    memcpy( key_, s, Rec::KEY_LEN );
  }

  RecordLoc( const RecordLoc & other )
    : loc_{other.loc_},
      host_{other.host_},
      disk_{other.disk_}
  {
    memcpy( key_, other.key(), Rec::KEY_LEN );
  }

  RecordLoc & operator=( const RecordLoc & other )
  {
    if ( this != &other ) {
      loc_ = other.loc();
      host_ = other.host();
      disk_ = other.disk();
      memcpy( key_, other.key(), Rec::KEY_LEN );
    }
    return *this;
  }

  /* Accessors */
  void copy( const uint8_t * s, uint64_t loc = 0,
	     uint32_t host = 0, uint32_t disk = 0 )
  {
    loc_ = loc;
    host_ = host;
    disk_ = disk;
    memcpy( key_, s, Rec::KEY_LEN );
  }
  const uint8_t * key( void ) const noexcept { return key_; }
  uint64_t loc( void ) const noexcept { return loc_; }
  uint32_t host( void ) const noexcept { return host_; }
  uint32_t disk( void ) const noexcept { return disk_; }

  /* methods for boost::sort */
  const char * data( void ) const noexcept { return (char *) key_; }
  unsigned char operator[]( size_t i ) const noexcept { return key_[i]; }
  size_t size( void ) const noexcept { return Rec::KEY_LEN; }

  /* comparison */
  comp_op( <, RecordLoc )
  comp_op( <=, RecordLoc )
  comp_op( >, RecordLoc )

  int compare( const uint8_t * k, uint64_t l ) const noexcept;
  int compare( const RecordLoc & b ) const noexcept;
};

std::ostream & operator<<( std::ostream & o, const RecordLoc & r );

inline void iter_swap ( RecordLoc * a, RecordLoc * b ) noexcept
{
  std::swap( *a, *b );
}

#endif /* RECORD_LOC_HH */
