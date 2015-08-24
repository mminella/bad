#ifndef RAW_VECTOR_HH
#define RAW_VECTOR_HH

#include <cstring>
#include <vector>
#include <utility>

template <typename T>
class RawVector
{
private:
  T * data_;
  size_t size_;
  std::vector<T> vec_;
  bool own_;

public:
  RawVector( void )
    : data_{nullptr}, size_{0}, vec_{}, own_{false}
  {}

  RawVector( T * d, size_t s, bool own = true ) noexcept
    : data_{d}, size_{s}, vec_{}, own_{own}
  {}

  RawVector( std::vector<T> && v ) noexcept
    : data_{v.data()} , size_{v.size()} , vec_{std::move( v )}, own_{false}
  {}

  RawVector( const RawVector & other ) = delete;
  RawVector & operator=( const RawVector & other ) = delete;

  RawVector( RawVector && other )
    : data_{other.data_} , size_{other.size_} , vec_{move( other.vec_ )}
    , own_{other.own_}
  {
    other.data_ = nullptr;
    other.size_ = 0;
    other.own_ = false;
  }

  RawVector & operator=( RawVector && other )
  {
    if ( this != &other ) {
      std::swap( data_, other.data_ );
      std::swap( size_, other.size_ );
      std::swap( vec_, other.vec_ );
      std::swap( own_, other.own_ );
    }
    return *this;
  }

  ~RawVector()
  {
    if ( data_ != nullptr and own_ ) { delete[] data_; }
  }

  T * data( void ) noexcept { return data_; }
  size_t size( void ) const noexcept { return size_; }
  size_t & size( void ) noexcept { return size_; }
  bool own( void ) const noexcept { return own_; }
  bool & own( void ) noexcept { return own_; }

  T & operator[]( size_t i ) { return data_[i]; }
  T & back( void ) { return data_[size_ - 1]; }

  T * begin( void ) noexcept { return data_; }
  const T * begin( void ) const noexcept { return data_; }

  T * end( void ) noexcept { return data_ + size_; }
  const T * end( void ) const noexcept { return data_ + size_; }
};

#endif /* RAW_VECTOR_HH */
