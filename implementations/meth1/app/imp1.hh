#ifndef IMP1_HH
#define IMP1_HH

#include "implementation.hh"

#include "file.hh"

/**
 * Imp1 defines the basic implementation strategy:
 * - No upfront work.
 * - Full linear scan for each record.
 */
class Imp1 : public Implementation {
private:
  File      data_;
  Record    last_;
  size_type size_;

public:
  /* constructor */
  Imp1(std::string file);

  /* destructor */
  ~Imp1() = default;

  /* forbid copy & move assignment */
  Imp1( const Imp1 & other ) = delete;
  Imp1 & operator=( const Imp1 & other ) = delete;
  Imp1( Imp1 && other ) = delete;
  Imp1 & operator=( const Imp1 && other ) = delete;

private:
  void DoInitialize( void );
  std::vector<Record> DoRead( size_type pos, size_type size );
  size_type DoSize( void );

  Record linear_scan( File & in, const Record & after );
};

#endif
