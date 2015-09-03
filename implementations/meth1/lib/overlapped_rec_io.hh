#ifndef OVERLAPPED_REC_IO_HH
#define OVERLAPPED_REC_IO_HH

#include <cstring>
#include <system_error>

#include "circular_io_rec.hh"

/**
 * Specializes CircularIORec to deal with Files.
 */
template<size_t rec_size>
class OverlappedRecordIO : public CircularIORec<rec_size>
{
private:
  File & file_;
  uint64_t fsize_;

public:
  OverlappedRecordIO( File & file )
    : CircularIORec<rec_size>( file )
    , file_{file}
    , fsize_{file.size()}
  {};

  /* no copy or move */
  OverlappedRecordIO( const OverlappedRecordIO & r ) = delete;
  OverlappedRecordIO & operator=( const OverlappedRecordIO & r ) = delete;
  OverlappedRecordIO( OverlappedRecordIO && r ) = delete;
  OverlappedRecordIO & operator=( OverlappedRecordIO && r ) = delete;

  void rewind( void )
  {
    file_.rewind();
    CircularIORec<rec_size>::start_read( fsize_ );
  }
};

#endif /* OVERLAPPED_REC_IO_HH */
