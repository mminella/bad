#include "record.hh"

#include "client.hh"
#include "remote_file.hh"

using namespace std;
using namespace meth1;

void RemoteFile::nextChunk( uint64_t chunkN )
{

  if ( offset_ < size_ ) {
    if ( chunkN == 0 or chunkN > chunkSize_ ) {
      chunkN = chunkSize_;
    }
    c_->sendRead( offset_, chunkN );
    offset_ += chunkN;
    readRPC_ = true;
  }
}

bool RemoteFile::eof( void ) const noexcept
{
  if ( offset_ >= size_ and onWire_ == 0 and inBuf_ == 0 and !readRPC_ ) {
    return true;
  } else {
    return false;
  }
}

void RemoteFile::copyWire( void )
{
  if ( onWire_ > 0 and onWire_ <= bufSize_ ) {
    if ( buf_ == nullptr ) {
      buf_ = new uint8_t[bufSize_ * Rec::SIZE];
    }
    c_->sock_.read_all( (char *) buf_, onWire_ * Rec::SIZE );
    inBuf_ = onWire_;
    onWire_ = 0;
    bufOffset_ = 0;
  }
}

void RemoteFile::nextRecord( uint64_t remaining )
{
  if ( onWire_ == 0 and inBuf_ == 0 ) {
    if ( !readRPC_ ) {
      nextChunk( remaining );
    }
    onWire_ = c_->recvRead();
    readRPC_ = false;
  }

  if ( onWire_ > 0 ) {
    if ( onWire_ <= bufSize_ ) {
      copyWire();
      nextChunk( remaining );
    } else {
      onWire_--;
      head_ = c_->readRecord();
      return;
    }
  }

  if ( inBuf_ > 0 ) {
    head_ = { buf_ + bufOffset_ };
    bufOffset_ += Rec::SIZE;
    inBuf_--;
    return;
  }
}

