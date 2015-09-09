#ifndef REC_LOADER_HH
#define REC_LOADER_HH

#include <memory>
#include <string>

#include "tune_knobs.hh"

#include "file.hh"
#include "overlapped_rec_io.hh"

#include "record.hh"

class RecLoader
{
public:
  using RecIO = OverlappedRecordIO<Rec::SIZE>;
  using RR = RecordS;

private:
  std::unique_ptr<File> file_;
  std::unique_ptr<RecIO> rio_;
  bool eof_;
  uint64_t loc_;

public:
  RecLoader( std::string fileName, int flags )
    : file_{new File( fileName, flags )}
    , rio_{new RecIO( *file_, Knobs::DISK_BLOCKS )}
    , eof_{false}
    , loc_{0}
  {}

  /* no copy */
  RecLoader( const RecLoader & ) = delete;
  RecLoader & operator=( const RecLoader & ) = delete;

  /* allow move */
  RecLoader( RecLoader && other )
    : file_{std::move( other.file_ )}
    , rio_{std::move( other.rio_ )}
    , eof_{other.eof_}
    , loc_{other.loc_}
  {}

  RecLoader & operator=( RecLoader && other )
  {
    if ( this != &other ) {
      file_ = std::move( other.file_ );
      rio_ = std::move( other.rio_ );
      eof_ = other.eof_;
      loc_ = other.loc_;
    }
    return *this;
  }

  int id( void ) const noexcept { return file_->fd_num(); }
  uint64_t records( void ) const noexcept { return file_->size() / Rec::SIZE; }
  bool eof( void ) const noexcept { return eof_; }
  void rewind( void );

  RecordPtr next_record( void );
  uint64_t filter( RR * r1, uint64_t size, const Record & after,
                   const RR * const curMin );
};

#endif /* REC_LOADER_HH */
