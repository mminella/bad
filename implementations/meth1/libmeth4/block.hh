#ifndef METH4_BLOCK_HH
#define METH4_BLOCK_HH

#include <cstdint>
#include <string>

/* Our block type used through-out method 4 */
class block_t
{
public:
  uint8_t * buf;
  std::size_t len;
  uint16_t bucket;

  /* < buffer, buffer_used, bucket_ID > */
  block_t( uint8_t * b, std::size_t l, uint16_t k )
    : buf{b}, len{l}, bucket{k}
  {}

  block_t( void )
    : buf{nullptr}, len{0}, bucket{65535}
  {}

  block_t( const block_t & other )
    : buf{other.buf}, len{other.len}, bucket{other.bucket}
  {}
};

#endif /* METH4_BLOCK_HH */
