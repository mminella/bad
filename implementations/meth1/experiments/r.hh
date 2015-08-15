#ifndef R_HH
#define R_HH

#include <cstdint>
#include <cstring>
#include <utility>

/* Alternative Record struct. */
struct R {
  uint64_t loc_ = 0;
  uint8_t * val_ = nullptr;
  uint8_t key_[10];

  void copy( const uint8_t * r, uint64_t i ) noexcept
  {
    loc_ = i;
    if ( val_ == nullptr ) { val_ = new uint8_t[90]; }
    memcpy( val_, r + 10, 90 );
    memcpy( key_, r, 10 );
  }

  R( void ) noexcept {}
  R( const unsigned char * r, uint64_t i ) { copy( r, i ); }
  R( const R & other ) {
    loc_ = other.loc_;
    val_ = other.val_;
    memcpy( key_, other.key_, 10 );
  }
  R & operator=( const R & other ) {
    loc_ = other.loc_;
    val_ = other.val_;
    memcpy( key_, other.key_, 10 );
    return *this;
  }
  R( R && other ) {
    loc_ = other.loc_;
    std::swap( val_, other.val_ );
    memcpy( key_, other.key_, 10 );
  }
  R & operator=( R && other )
  {
    loc_ = other.loc_;
    std::swap( val_, other.val_ );
    memcpy( key_, other.key_, 10 );
    return *this;
  }
  ~R( void ) noexcept
  {
    if ( val_ != nullptr ) { delete val_; }
  }

  /* Accessors */
  const uint8_t * key( void ) const noexcept { return key_; }
  const uint8_t * val( void ) const noexcept { return val_; }
  uint64_t loc( void ) const noexcept { return loc_; }

  /* boost sort */
  const char * data( void ) const noexcept { return (char *) key_; }
  unsigned char operator[]( size_t i ) const noexcept { return key_[i]; }
  size_t size( void ) const noexcept { return 10; }

  /* comparison */
  bool operator<( const R & b ) const noexcept
  {
    return compare( b ) < 0;
  }

  int compare( const R & b ) const noexcept
  {
    return compare( b.key(), b.loc() );
  }

  int compare( const unsigned char * bkey, uint64_t bloc ) const noexcept
  {
    for ( size_t i = 0; i < 10; i++ ) {
      if ( key()[i] != bkey[i] ) {
        return key()[i] - bkey[i];
      }
    }
    if ( loc() < bloc ) {
      return -1;
    }
    if ( loc() > bloc ) {
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
