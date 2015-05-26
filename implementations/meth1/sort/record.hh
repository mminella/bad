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

  enum limit_t { MAX, MIN };

private:
  uint64_t offset_;

  /* Private storage for copying record to */
  uint8_t key_[KEY_LEN];
  uint8_t val_[VAL_LEN];

  /* Can simply point to existing storage */
  const uint8_t * key_r_;
  const uint8_t * val_r_;

  /* Did we make a copy? */
  bool copied_;

  /* Copy given record into our own storage */
  void copy( const Record & other  );

public:
  /* Construct an empty record. */
  Record( limit_t lim );

  /* Construct from c string read from disk. By default simply uses the storage
   * passed in (so needs to remain valid for life of Record). But if `copy` is
   * true, an internal private copy of the record will be made. */
  Record( uint64_t offset, const char * s, bool copy = false);

  /* Construct from string read from disk */
  Record( uint64_t offset, std::string s, bool copy = false )
    : Record( offset, s.c_str(), copy ) {}

  /* Allow move, copying and assignment */
  Record( Record && other );
  Record( const Record & other, bool deep_copy = false);
  Record & operator=( const Record & other );

  /* Destructor */
  ~Record( void );

  /* Clone method to make a deep copy */
  Record clone( void );

  /* Accessors */
  uint64_t offset( void ) const { return offset_; }
  const uint8_t * key( void ) const { return key_r_; }
  const uint8_t * value( void ) const { return val_r_; }

  /* Serialization */
  std::string str( void ) const;

  /* Comparison */
  bool operator<( const Record & b )
  {
    return std::memcmp( key(), b.key(), KEY_LEN ) < 0 ? true : false;
  }

  bool operator<=( const Record & b )
  {
    return std::memcmp( key(), b.key(), KEY_LEN ) <= 0 ? true : false;
  }
};

#endif /* RECORD_HH */
