#ifndef REC_HH
#define REC_HH

/*
 * Sort record type.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <system_error>

static constexpr size_t KEY_BYTES = 10;
static constexpr size_t VAL_BYTES = 90;
static constexpr size_t REC_BYTES = KEY_BYTES + VAL_BYTES;

// array for record values -- bump allocator (no free, not thread-safe)
static char * vals;
static char * cur_v;

void setup_value_storage( size_t nrecs )
{
  cur_v = vals = new char[ nrecs * VAL_BYTES ];
}

// our record struct
class Rec
{
private:
  // Pulling the key inline with Rec improves sort performance by 2x. Appears
  // to be due to saving an indirection during sort.
  char key_[KEY_BYTES];
  char * val_;

  inline void checked_fread( void * ptr, size_t size, size_t nmemb, FILE * f )
  {
    size_t n = fread( ptr, size, nmemb, f );
    if ( n != 1 ) {
      throw std::runtime_error( "Couldn't read record" );
    }
  }

public:
  Rec() : val_{nullptr} {}

  inline void read( FILE *fdi )
  {
    val_ = cur_v;
    cur_v += VAL_BYTES;
    checked_fread( key_, KEY_BYTES, 1, fdi );
    checked_fread( val_, VAL_BYTES, 1, fdi );
  }

  inline size_t write( FILE *fdo )
  {
    size_t n = 0;
    n += fwrite( key_, KEY_BYTES, 1, fdo );
    n += fwrite( val_, VAL_BYTES, 1, fdo );
    return n;
  }

  inline bool operator<( const Rec & b ) const
  {
    return compare( b ) < 0 ? true : false;
  }

  inline int compare( const Rec & b ) const
  {
    return memcmp( key_, b.key_, KEY_BYTES );
  }

  inline const char * data( void ) const
  {
    return key_;
  }

  inline unsigned char operator[]( size_t offset ) const
  {
    return key_[offset];
  }

  inline size_t size( void ) const
  {
    return KEY_BYTES;
  }
} __attribute__((packed));

#endif /* REC_HH */
