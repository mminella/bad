#ifndef REMOTE_FILE_HH
#define REMOTE_FILE_HH

#include <exception>
#include <memory>

#include "circular_io_rec.hh"
#include "record.hh"

#include "client.hh"

/**
 * Strategy 1.
 * - No upfront work.
 * - Full linear scan for each record.
 */
namespace meth1
{

/**
 * RemoteFile is a helper wrapper around a Client presents a stateful File
 * interface and performs read-ahead.
 */
class RemoteFile
{
private:
  Client * c_;
  CircularIORec<Rec::SIZE> * buf_;

  uint64_t chunkSize_;

  bool start_;
  uint64_t ioPos_;
  uint64_t pos_;
  uint64_t size_;

  RecordPtr head_;

public:
  RemoteFile( Client & c, uint64_t chunkSize, uint64_t bufSize )
    : c_{&c}
    , buf_{new CircularIORec<Rec::SIZE>( c.socket(), bufSize,
        (uint64_t) c.socket().fd_num() )}
    , chunkSize_{chunkSize}
    , start_{true}
    , ioPos_{0}
    , pos_{0}
    , size_{0}
    , head_{(const char *) nullptr}
  {
    buf_->set_io_drained_cb( [this]() {
      this->nextChunk();
    } );
  }

  /* no copy */
  RemoteFile( const RemoteFile & other ) = delete;
  RemoteFile & operator=( RemoteFile & other ) = delete;

  /* no move */
  RemoteFile( RemoteFile && other ) = delete;
  RemoteFile & operator=( RemoteFile && other ) = delete;

  ~RemoteFile( void )
  {
    if ( buf_ != nullptr ) { delete buf_; }
  }

  void sendSize( void ) { c_->sendSize(); }
  uint64_t recvSize( void ) { size_ = c_->recvSize(); return size_; }

  void nextChunk( void )
  {
    if ( size_ == 0 ) {
      sendSize();
      recvSize();
    }
    if ( ioPos_ < size_ ) {
      uint64_t siz = std::min( chunkSize_, size_ - ioPos_ );
      c_->sendRead( ioPos_, siz );
      ioPos_ += siz;
    }
  }

  void nextRecord( void )
  {
    if ( eof() ) {
      return;
    }

    const char * recStr = nullptr;
    if ( start_ ) {
      recStr = nullptr;
      start_ = false;
    } else {
      recStr = buf_->next_record();
    }

    if ( recStr == nullptr ) {
      uint64_t nrecs = c_->recvRead();
      buf_->reset_rec_count();
      buf_->start_read( nrecs * Rec::SIZE );
      recStr = buf_->next_record();
    }

    head_ = {recStr};
    pos_++;

    if ( pos_ == size_ ) {
      // drain the final nullptr
      recStr = buf_->next_record();
      if ( recStr != nullptr ) {
        throw new std::runtime_error( "not actually eof" );
      }
    }
  }

  RecordPtr curRecord( void ) const noexcept { return head_; }
  bool eof( void ) const noexcept
  {
    return pos_ >= size_;
  }

  bool operator>( const RemoteFile & b ) const noexcept
  {
    return head_ > b.head_;
  }
};

/* Pointer wrapper around a RemoteFile. */
class RemoteFilePtr
{
private:
  RemoteFile * rf_;

public:
  RemoteFilePtr( RemoteFile * rf )
    : rf_{rf}
  {}

  RemoteFile * rf( void ) const noexcept { return rf_; }

  RecordPtr curRecord( void ) const noexcept { return rf_->curRecord(); }
  void nextRecord( void ) { return rf_->nextRecord(); }
  bool eof( void ) const noexcept { return rf_->eof(); }

  bool operator>( const RemoteFilePtr & b ) const noexcept
  {
    return *rf_ > *b.rf_;
  }
};
}

#endif /* REMOTE_FILE_HH */
