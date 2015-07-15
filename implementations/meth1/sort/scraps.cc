// Scrap C++ code

class RecordPtr
{
public:
  Record & r_;
  const char * key_;

  /* Construct */
  RecordPtr( Record & r ) : r_{r}, key_{r_.key()} {};

  /* Allow copy */
  RecordPtr( const RecordPtr & other ) = default;
  RecordPtr & operator=( const RecordPtr & other ) = default;

  /* Allow move */
  RecordPtr( RecordPtr && other ) : r_{other.r_}, key_{r_.key()} {};
  RecordPtr & operator=( RecordPtr && other )
  {
    r_ = other.r_;
    key_ = other.key_;
    return *this;
  }

  /* Comparison */
  bool operator<( const RecordPtr & b ) const
  {
    return compare( b ) < 0 ? true : false;
  }

  bool operator<=( const RecordPtr & b ) const
  {
    return compare( b ) <= 0 ? true : false;
  }

  bool operator>( const RecordPtr & b ) const
  {
    return compare( b ) > 0 ? true : false;
  }

  /* we compare on key first, and then on diskloc_ */
  int compare( const RecordPtr & b ) const
  {
    int cmp = std::memcmp( key_, b.key_, Record::KEY_LEN );
    if ( cmp == 0 ) {
      if ( r_.diskloc() < b.r_.diskloc() ) {
        cmp = -1;
      }
      if ( r_.diskloc() > b.r_.diskloc() ) {
        cmp = 1;
      }
    }
    return cmp;
  }
};

#else /* __RECORD_V0 */

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

  /* Can simply point to existing storage */
  key_t * key_r_;
  val_t * val_r_;

  /* Did we make a copy? */
  bool copied_;

  /* Copy external storage into our own storage */
  void copy( void );

public:
  /* Construct an empty record. */
  Record( limit_t lim );

  /* Construct from c string read from disk. By default simply uses the storage
   * passed in (so needs to remain valid for life of Record). But if `copy` is
   * true, an internal private copy of the record will be made. */
  Record( const char * s, size_type diskloc, bool copy = false );

  /* Allow copy */
  Record( const Record & other, bool deep_copy = false );
  Record & operator=( const Record & other );

  /* Allow move */
  Record( Record && other );
  Record & operator=( Record && other );

  /* Destructor */
  ~Record( void );

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

  bool operator>( const Record & b ) const
  {
    return compare( b ) > 0 ? true : false;
  }

  /* we compare on key first, and then on diskloc_ */
  int compare( const Record & b ) const
  {
    int cmp = std::memcmp( key(), b.key(), KEY_LEN );
    if ( cmp == 0 ) {
      if ( diskloc_ < b.diskloc_ ) {
        cmp = -1;
      }
      if ( diskloc_ > b.diskloc_ ) {
        cmp = 1;
      }
    }
    return cmp;
  }

public:
  static Record ParseRecord( const char * s, size_type diskloc, bool copy );

  static Record ParseRecord( std::string s, size_type diskloc, bool copy )
  {
    return ParseRecord( s.c_str(), diskloc, copy );
  }

  static Record ParseRecordWithLoc( const char * s, bool copy = false );

  static Record ParseRecordWithLoc( std::string s, bool copy = false )
  {
    return ParseRecordWithLoc( s.c_str(), copy );
  }
};

#endif /* RECORD_V0 */

#else

/* Construct an empty record. Guaranteed to be the maximum record. */
Record::Record( limit_t lim )
  : diskloc_{0}
  , key_r_{new key_t[KEY_LEN]}
  , val_r_{new val_t[VAL_LEN]}
  , copied_{true}
{
  if ( lim == MAX ) {
    memset( key_r_, 0xFF, KEY_LEN );
  } else {
    memset( key_r_, 0x00, KEY_LEN );
  }
}

/* Copy external storage into our own storage */
void Record::copy( void )
{
  key_t *k = new char[KEY_LEN];
  val_t *v = new char[VAL_LEN];
  memcpy( k, key_r_, KEY_LEN );
  memcpy( v, val_r_, VAL_LEN );
  key_r_ = k;
  val_r_ = v;
  copied_ = true;
}

/* Construct from c string read from disk */
Record::Record( const char * s, size_type loc, bool copy )
  : diskloc_{loc}
  , key_r_{(char *) s}
  , val_r_{(char *) s + KEY_LEN}
  , copied_{copy}
{
  if ( copied_ ) {
    Record::copy();
  }
}

/* Copy constructor */
Record::Record( const Record & other, bool deep_copy )
  : diskloc_{other.diskloc_}
  , key_r_{other.key_r_}
  , val_r_{other.val_r_}
  , copied_{other.copied_ or deep_copy}
{
  if ( copied_ ) {
    copy();
  }
}

/* Copy assignment */
Record & Record::operator=( const Record & other )
{
  if ( this != &other ) {
    diskloc_ = other.diskloc_;
    key_r_ = other.key_r_;
    val_r_ = other.val_r_;
    copied_ = other.copied_;

    if ( copied_ ) {
      copy();
    }
  }
  return *this;
}

/* Move constructor */
Record::Record( Record && other )
  : diskloc_{other.diskloc_}
  , key_r_{other.key_r_}
  , val_r_{other.val_r_}
  , copied_{other.copied_}
{
  if ( this != &other ) {
    other.key_r_ = nullptr;
    other.val_r_ = nullptr;
    other.copied_ = false;
  }
}

/* Move assignment */
Record & Record::operator=( Record && other )
{
  if ( this != &other ) {
    if ( copied_ ) {
      delete key_r_;
      delete val_r_;
    }
    diskloc_ = other.diskloc_;
    key_r_ = other.key_r_;
    val_r_ = other.val_r_;
    copied_ = other.copied_;

    other.key_r_ = nullptr;
    other.val_r_ = nullptr;
    other.copied_ = false;
  }
  return *this;
}

/* Destructor */
Record::~Record( void )
{
  if ( copied_ ) {
    delete key_r_;
    delete val_r_;
  }
}

/* To string */
string Record::str( loc_t locinfo ) const
{
  string buf{key_r_, KEY_LEN};
  buf.append( val_r_, VAL_LEN );
  if ( locinfo == WITH_LOC ) {
    buf.append( reinterpret_cast<const char *>( &diskloc_ ),
                sizeof( size_type ) );
  }
  return buf;
}

Record Record::ParseRecord( const char * s, Record::size_type diskloc,
                            bool copy )
{
  return Record{s, diskloc, copy};
}

Record Record::ParseRecordWithLoc( const char * s, bool copy )
{
  Record r{s, 0, copy};
  r.diskloc_ = *reinterpret_cast<const size_type *>( s + SIZE );
  return r;
}

#endif /* RECORD_V0 */
