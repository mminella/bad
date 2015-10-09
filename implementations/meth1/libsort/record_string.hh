#ifndef RECORD_STRING_HH
#define RECORD_STRING_HH

#include <cstdint>

#include "io_device.hh"

#include "record_common.hh"

/* Represents a contiguous record (i.e., read directly from disk). */
class RecordString
{
private:
  char r_[Rec::SIZE];

public:
  /* We just type-cast for now to use a RecordString. */
  RecordString( void );

  /* Accessors */
  const uint8_t * key( void ) const noexcept { return (uint8_t *) r_; }
  const uint8_t * val( void ) const noexcept { return key() + Rec::KEY_LEN; }

  /* methods for boost::sort */
  const char * data( void ) const noexcept { return r_; }
  unsigned char operator[]( size_t i ) const noexcept { return r_[i]; }
  size_t size( void ) const noexcept { return Rec::KEY_LEN; }

  /* comparison */
  comp_op( <, RecordString )
  comp_op( <=, RecordString )
  comp_op( >, RecordString )

  int compare( const uint8_t * k ) const noexcept;
  int compare( const RecordString & b ) const noexcept;
} __attribute__(( packed ));

std::ostream & operator<<( std::ostream & o, const RecordString & r );

inline void iter_swap ( RecordString * a, RecordString * b ) noexcept
{
  std::swap( *a, *b );
}

#endif /* RECORD_STRING_HH */
