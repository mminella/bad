#ifndef OVERLAPPED_REC_IO_HH
#define OVERLAPPED_REC_IO_HH

#include <cstring>
#include <system_error>

#include "tune_knobs.hh"

#include "circular_io_rec.hh"

/* Ensure size_t is large enough */
static_assert( sizeof( off_t ) <= sizeof( size_t ), "off_t <= size_t" );

/**
 * Specializes CircularIORec to deal with Files.
 */
template<size_t rec_size>
class OverlappedRecordIO : public CircularIORec<rec_size>
{
private:
  File & file_;
  off_t fsize_;

public:
  OverlappedRecordIO( File & file, size_t blocks = Knobs::DISK_BLOCKS )
    : CircularIORec<rec_size>{file, blocks, file.fd_num()}
    , file_{file}
    , fsize_{file.size()}
  {
    if ( fsize_ < 0 ) {
      throw std::runtime_error( "file size less then zero" );
    }
  };

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
