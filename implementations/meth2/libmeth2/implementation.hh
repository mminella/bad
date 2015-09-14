#ifndef IMPLEMENTATION_HH
#define IMPLEMENTATION_HH

#include <cstdint>
#include <vector>
#include <utility>

#include "raw_vector.hh"

#include "record.hh"

/**
 * Defines the interface any implementation must satisfy.
 */
class Implementation
{
public: using RR = Record;
  using RecV = std::vector<Record>;

private:
  virtual void DoInitialize( void ) = 0;

  virtual RecV DoRead( uint64_t pos, uint64_t size ) = 0;

  virtual uint64_t DoSize( void ) = 0;

public:
  /* Destructor */
  virtual ~Implementation() {}

  /* Initialization routine */
  void Initialize( void ) { DoInitialize(); };

  /* Read a contiguous subset of the file starting from specified position. */
  RecV Read( uint64_t pos, uint64_t size )
  {
    return DoRead( pos, size );
  };

  /* Return the the number of records on disk */
  uint64_t Size( void ) { return DoSize(); };
};

/**
 * Defines the interface any implementation must satisfy.
 */
class Implementation2
{
public:
  using RR = RecordS;
  using RecV = RawVector<RR>;

private:
  virtual void DoInitialize( void ) = 0;

  virtual RecV DoRead( uint64_t pos, uint64_t size ) = 0;

  virtual uint64_t DoSize( void ) = 0;

public:
  /* Destructor */
  virtual ~Implementation2() {}

  /* Initialization routine */
  void Initialize( void ) { DoInitialize(); };

  /* Read a contiguous subset of the file starting from specified position. */
  RecV Read( uint64_t pos, uint64_t size )
  {
    return DoRead( pos, size );
  };

  /* Return the the number of records on disk */
  uint64_t Size( void ) { return DoSize(); };
};

#endif /* IMPLEMENTATION_HH */
