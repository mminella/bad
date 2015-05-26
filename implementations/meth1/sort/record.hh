#ifndef RECORD_HH
#define RECORD_HH

#include <cstring>
#include <string>

/**
 * Represent a record in the sort benchmark. Copies data on initialization and
 * stores internally in statically sized arrays.
 */
class Record {
public:
  static constexpr size_t SIZE = 100;
  static constexpr size_t KEY_LEN = 10;
  static constexpr size_t VAL_LEN = 90;

private:
  uint8_t key_[KEY_LEN];
  uint8_t value_[VAL_LEN];

public:
  /* construct from c string read from disk */
  Record( const char * s );

  /* construct from string read from disk */
  Record( std::string s ) : Record( s.c_str() ) {};

  /* allow move, copying and assignment */
  Record( Record && other ) = default;
  Record( const Record & other ) = default;
  Record & operator=( const Record & other ) = default;

  /* accessors */
  const uint8_t * key( void ) const { return key_; };
  const uint8_t * value( void ) const { return value_; };

  /* serialization */
  std::string str( void );

  /* comparison */
  bool operator<( const Record & b )
  {
    return std::memcmp( key(), b.key(), KEY_LEN ) < 0 ? true : false;
  }
};

#endif /* RECORD_HH */
