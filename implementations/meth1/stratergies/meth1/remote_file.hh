#ifndef REMOTE_FILE_HH
#define REMOTE_FILE_HH

#include <memory>

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
  uint64_t chunkSize_;
  uint64_t  bufSize_;

  uint64_t size_ = 0;
  uint64_t offset_ = 0;

  bool readRPC_ = false;
  uint64_t onWire_ = 0;
  uint64_t inBuf_ = 0;
  std::unique_ptr<uint8_t> buf_{};
  uint64_t bufOffset_ = 0;

  RecordPtr head_;

  void copyWire( void );

public:
  RemoteFile( Client & c, uint64_t chunkSize, uint64_t bufSize )
    : c_{&c}
    , chunkSize_{chunkSize}
    , bufSize_{bufSize}
    , head_{(const char *) nullptr}
  {};

  /* no copy */
  RemoteFile( const RemoteFile & other ) = delete;
  RemoteFile & operator=( RemoteFile & other ) = delete;

  /* allow move */
  RemoteFile( RemoteFile && other )
    : c_{other.c_}
    , chunkSize_{other.chunkSize_}
    , bufSize_{other.bufSize_}
    , size_{other.size_}
    , offset_{other.offset_}
    , readRPC_{other.readRPC_}
    , onWire_{other.onWire_}
    , inBuf_{other.inBuf_}
    , buf_{std::move( other.buf_ )}
    , bufOffset_{other.bufOffset_}
    , head_{other.head_}
  {}

  RemoteFile & operator=( RemoteFile && other )
  {
    if ( this != &other ) {
      c_ = other.c_;
      chunkSize_ = other.chunkSize_;
      bufSize_ = other.bufSize_;
      size_ = other.size_;
      offset_ = other.offset_;
      readRPC_ = other.readRPC_;
      onWire_ = other.onWire_;
      inBuf_ = other.inBuf_;
      buf_ = std::move( other.buf_ );
      bufOffset_ = other.bufOffset_;
      head_ = other.head_;
    }
    return *this;
  }

  void sendSize( void ) { c_->sendSize(); }
  void recvSize( void ) { size_ = c_->recvSize(); }

  RecordPtr curRecord( void ) const noexcept { return head_; }
  void nextRecord( uint64_t remaining = 0 );
  void nextChunk( uint64_t chunkN = 0 );
  bool eof( void ) const noexcept;

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
  RemoteFilePtr( RemoteFile & rf )
    : rf_{&rf}
  {}

  RemoteFile & rf( void ) const noexcept { return *rf_; }

  bool eof( void ) const noexcept { return rf_->eof(); }
  RecordPtr curRecord( void ) const noexcept { return rf_->curRecord(); }
  void nextRecord( uint64_t remaining = 0 )
  {
    return rf_->nextRecord( remaining );
  }

  bool operator>( const RemoteFilePtr & b ) const noexcept
  {
    return rf_ > b.rf_;
  }
};
}

#endif /* REMOTE_FILE_HH */
