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

public:
  RawVector( void )
    : data_{nullptr}, size_{0}, vec_{}
  {}

  RawVector( T * d, size_t s ) noexcept
    : data_{d}, size_{s}, vec_{}
  {}

  RawVector( std::vector<T> && v ) noexcept
    : data_{v.data()} , size_{v.size()} , vec_{std::move( v )}
  {}

  RawVector( const RawVector & other ) = delete;
  RawVector & operator=( const RawVector & other ) = delete;

  RawVector( RawVector && other )
    : data_{other.data_} , size_{other.size_} , vec_{move( other.vec_ )}
  {
    other.data_ = nullptr;
    other.size_ = 0;
  }

  RawVector & operator=( RawVector && other )
  {
    if ( this != &other ) {
      std::swap( data_, other.data_ );
      std::swap( size_, other.size_ );
      std::swap( vec_, other.vec_ );
    }
    return *this;
  }

  ~RawVector()
  {
    if ( data_ != nullptr and vec_.size() == 0 ) { delete[] data_; }
  }

  size_t size( void ) const noexcept { return size_; }

  T & operator[]( size_t i ) { return data_[i]; }
  T & back( void ) { return data_[size_ - 1]; }

  T * begin( void ) noexcept { return data_; }
  const T * begin( void ) const noexcept { return data_; }

  T * end( void ) noexcept { return data_ + size_; }
  const T * end( void ) const noexcept { return data_ + size_; }
};

#endif /* RAW_VECTOR_HH */
