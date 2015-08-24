#ifndef REMOTE_FILE_HH
#define REMOTE_FILE_HH

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
  uint8_t * buf_ = nullptr;
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
}

#endif /* REMOTE_FILE_HH */
