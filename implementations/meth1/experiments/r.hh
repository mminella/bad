#ifndef R_HH
#define R_HH

#include <cstdint>
#include <cstring>
#include <utility>

/* Alternative Record struct. */
struct R {
  uint64_t loc = 0;
  char * val = nullptr;
  unsigned char key[10];

  void init( const unsigned char * r, uint64_t i ) noexcept
  {
    loc = i;
    if ( val == nullptr ) { val = new char[90]; }
    memcpy( val, r + 10, 90 );
    memcpy( key, r, 10 );
  }

  R( void ) noexcept {}
  R( const unsigned char * r, uint64_t i ) { init( r, i ); }
  R( const R & other ) {
    loc = other.loc;
    val = other.val;
    memcpy( key, other.key, 10 );
  }
  R & operator=( const R & other ) {
    loc = other.loc;
    val = other.val;
    memcpy( key, other.key, 10 );
    return *this;
  }
  R( R && other ) {
    loc = other.loc;
    std::swap( val, other.val );
    memcpy( key, other.key, 10 );
  }
  R & operator=( R && other )
  {
    loc = other.loc;
    std::swap( val, other.val );
    memcpy( key, other.key, 10 );
    return *this;
  }
  ~R( void ) noexcept
  {
    if ( val != nullptr ) { delete val; }
  }

  const char * data( void ) const noexcept { return reinterpret_cast<const char *>( key ); }
  unsigned char operator[]( size_t i ) const noexcept { return key[i]; }
  size_t size( void ) const noexcept { return 10; }

  bool operator<( const R & b ) const noexcept
  {
    return compare( b ) < 0;
  }

  int compare( const R & b ) const noexcept
  {
    return compare( b.key, b.loc );
  }

  int compare( const unsigned char * bkey, uint64_t bloc ) const noexcept
  {
    for ( size_t i = 0; i < 10; i++ ) {
      if ( key[i] != bkey[i] ) {
        return key[i] - bkey[i];
      }
    }
    if ( loc < bloc ) {
      return -1;
    }
    if ( loc > bloc ) {
      return 1;
    }
    return 0;
  }
};

void iter_swap ( R * a, R * b ) noexcept
{
  std::swap( *a, *b );
}

#endif /* R_HH */
