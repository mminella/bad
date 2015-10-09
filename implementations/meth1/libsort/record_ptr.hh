#ifndef RECORD_PTR_HH
#define RECORD_PTR_HH

#include <cstdint>

#include "io_device.hh"

#include "record_common.hh"

/* Wrapper around a string (and location) to enable comparison. Doens't manage
 * it's own storage. */
class RecordPtr
{
private:
#if WITHLOC == 1
  uint64_t loc_;
#endif
  const uint8_t * r_;

public:
  RecordPtr( const uint8_t * r, uint64_t loc = 0 )
#if WITHLOC == 1
    : loc_{loc}
    , r_{r}
  {}
#else
    : r_{r}
  { (void) loc; }
#endif

  RecordPtr( const char * r, uint64_t loc = 0 )
    : RecordPtr( (const uint8_t *) r, loc )
  {}

  RecordPtr( const RecordPtr & rptr )
#if WITHLOC == 1
    : loc_{ rptr.loc_ }
    , r_{ rptr.r_ }
#else
    : r_{ rptr.r_ }
#endif
  {}

  RecordPtr & operator=( const RecordPtr & rptr )
  {
#if WITHLOC == 1
    loc_ = rptr.loc_;
#endif
    r_ = rptr.r_;
    return *this;
  }

  /* Accessors */
  bool isNull( void ) const noexcept { return r_ == nullptr; }
  const uint8_t * key( void ) const noexcept { return r_; }
  const uint8_t * val( void ) const noexcept { return r_ + Rec::KEY_LEN; }
#if WITHLOC == 1
  uint64_t loc( void ) const noexcept { return loc_; }
#else
  uint64_t loc( void ) const noexcept { return 0; }
#endif

  /* methods for boost::sort */
  const char * data( void ) const noexcept { return (char *) r_; }
  unsigned char operator[]( size_t i ) const noexcept { return r_[i]; }
  size_t size( void ) const noexcept { return Rec::KEY_LEN; }

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
  int compare( const char * k, uint64_t l ) const noexcept;
  int compare( const Record & b ) const noexcept;
  int compare( const RecordS & b ) const noexcept;
  int compare( const RecordPtr & b ) const noexcept;

  /* Write to IO device */
  void write( IODevice & io, Rec::loc_t locinfo = Rec::NO_LOC ) const
  {
    io.write_all( (const char *) key(), Rec::KEY_LEN );
    io.write_all( (const char *) val(), Rec::VAL_LEN );
    if ( locinfo == Rec::WITH_LOC ) {
      uint64_t l = loc();
      io.write_all( (const char *) &l, Rec::LOC_LEN );
    }
  }
};

std::ostream & operator<<( std::ostream & o, const RecordPtr & r );

inline void iter_swap ( RecordPtr * a, RecordPtr * b ) noexcept
{
  std::swap( *a, *b );
}

#endif /* RECORD_PTR_HH */
