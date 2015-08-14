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
  const unsigned char * r_;

public:
  RecordPtr( const char * r, uint64_t loc )
    : loc_{loc}
    , r_{(unsigned char *) r}
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

  const unsigned char * key( void ) const noexcept { return r_; }
  const unsigned char * val( void ) const noexcept { return r_ + Rec::KEY_LEN; }
  uint64_t loc( void ) const noexcept { return loc_; }

  /* comparison (with Record & RecordPtr) */
  comp_op( <, Record )
  comp_op( <, RecordPtr )
  comp_op( <=, Record )
  comp_op( <=, RecordPtr )
  comp_op( >, Record )
  comp_op( >, RecordPtr )

  int compare( const Record & b ) const;
  int compare( const RecordPtr & b ) const;
};

#endif /* RECORD_PTR_HH */
