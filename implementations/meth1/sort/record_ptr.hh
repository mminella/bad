#ifndef RECORD_PTR_HH
#define RECORD_PTR_HH

#include <cstdint>

#include "record_common.hh"

/* Wrapper around a string (and location) to enable comparison. Doens't manage
 * it's own storage. */
class RecordPtr
{
private:
  uint64_t loc_;
  const uint8_t * r_;

public:
  RecordPtr( const uint8_t * r, uint64_t loc )
    : loc_{loc} , r_{r}
  {}

  RecordPtr( const char * r, uint64_t loc )
    : RecordPtr( (const uint8_t *) r, loc )
  {}

  RecordPtr( const RecordPtr & rptr )
    : loc_{ rptr.loc_ }
    , r_{ rptr.r_ }
  {}

  RecordPtr & operator=( const RecordPtr & rptr )
  {
    if ( this != &rptr ) {
      loc_ = rptr.loc_;
      r_ = rptr.r_;
    }
    return *this;
  }

  /* Accessors */
  const uint8_t * key( void ) const noexcept { return r_; }
  const uint8_t * val( void ) const noexcept { return r_ + Rec::KEY_LEN; }
  uint64_t loc( void ) const noexcept { return loc_; }

  /* comparison */
  comp_op( <, Record )
  comp_op( <, RecordS )
  comp_op( <, RecordPtr )
  comp_op( <=, Record )
  comp_op( <=, RecordS )
  comp_op( <=, RecordPtr )
  comp_op( >, Record )
  comp_op( >, RecordS )
  comp_op( >, RecordPtr )

  int compare( const uint8_t * k, uint64_t l ) const noexcept;
  int compare( const Record & b ) const noexcept;
  int compare( const RecordS & b ) const noexcept;
  int compare( const RecordPtr & b ) const noexcept;
};

#endif /* RECORD_PTR_HH */
