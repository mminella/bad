#ifndef REMOTE_FILE_HH
#define REMOTE_FILE_HH

#include "record.hh"

#include "client.hh"

/**
 * Strategy 2.
 */
namespace meth2
{

// TODO: dynamically adjust low limit based on number of backends and available memory
static const size_t LOW_LIMIT = 1048576; // 100MB

/**
 * RemoteFile is a helper wrapper around a Client presents a stateful File
 * interface and performs read-ahead.
 */
class RemoteFile
{
private:
  Client * c_;
  uint64_t chunkSize_;
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
  RemoteFile( Client & c, uint64_t chunkSize )
    : c_{&c}, chunkSize_{chunkSize}, head_{(const char *) nullptr}
  {};

  void drain();

  uint64_t seek(uint64_t newOffset) {
    uint64_t oldOffset = offset_;
    offset_ = newOffset;
    return oldOffset;
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
}

#endif /* REMOTE_FILE_HH */
