#ifndef RECORD_HH
#define RECORD_HH

#include <cstring>
#include <string>

/**
 * Represent a record in the sort benchmark. Copies data on initialization and
 * stores internally in statically sized arrays.
 */
class Record
{
public:
  using size_type = uint64_t;
  using key_t = char;
  using val_t = char;

  static constexpr size_t SIZE = 100;
  static constexpr size_t KEY_LEN = 10;
  static constexpr size_t VAL_LEN = 90;

  static constexpr size_t LOC_LEN = sizeof( size_type );
  static constexpr size_t SIZE_WITH_LOC = SIZE + LOC_LEN;

  /* A Max or minimum record type. */
  enum limit_t { MAX, MIN };

  /* Should we include or not include disk location information? */
  enum loc_t { WITH_LOC, NO_LOC };

private:
  size_type diskloc_;

  /* Private storage for copying record to */
  key_t key_[KEY_LEN];
  val_t val_[VAL_LEN];

  /* Can simply point to existing storage */
  const key_t * key_r_;
  const val_t * val_r_;

  /* Did we make a copy? */
  bool copied_;

  /* Copy external storage into our own storage */
  void copy( void );

  /* Construct from c string read from disk. By default simply uses the storage
   * passed in (so needs to remain valid for life of Record). But if `copy` is
   * true, an internal private copy of the record will be made. */
  Record( const char * s, size_type diskloc, bool copy = false );

public:
  /* Construct an empty record. */
  Record( limit_t lim );

  /* Allow copy, but no move */
  Record( const Record & other, bool deep_copy = false );
  Record & operator=( const Record & other );

  /* Destructor */
  ~Record( void ) = default;

  /* Clone method to make a deep copy */
  Record clone( void ) { return {*this, true}; };

  /* Accessors */
  size_type diskloc( void ) const { return diskloc_; }
  const key_t * key( void ) const { return key_r_; }
  const key_t * value( void ) const { return val_r_; }

  /* Serialization to string. By default, disk location data is included,  */
  std::string str( loc_t locinfo ) const;

  /* Comparison */
  bool operator<( const Record & b ) const
  {
    return compare( b ) < 0 ? true : false;
  }

  bool operator<=( const Record & b ) const
  {
    return compare( b ) <= 0 ? true : false;
  }

  int compare( const Record & b ) const
  {
    return std::memcmp( key(), b.key(), KEY_LEN );
  }

public:
  static Record ParseRecord( const char * s, size_type diskloc,
                             bool copy = false );

  static Record ParseRecord( std::string s, size_type diskloc,
                             bool copy = false )
  {
    return ParseRecord( s.c_str(), diskloc, copy );
  }

  static Record ParseRecordWithLoc( const char * s, bool copy = false );

  static Record ParseRecordWithLoc( std::string s, bool copy = false )
  {
    return ParseRecordWithLoc( s.c_str(), copy );
  }
};

#endif /* RECORD_HH */
