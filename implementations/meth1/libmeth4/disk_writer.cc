#include <sys/stat.h>
#include <sys/types.h>

#include "disk_writer.hh"
#include "sync_print.hh"

using namespace std;

const string BUCKET_DIR = "buckets";
const string BUCKET_EXT = "bucket";

// De-allocation helper
static void freeBlock( uint8_t * buf )
{
  free( buf );
}

DiskWriter::DiskWriter( ClusterMap & cluster, uint8_t diskID, string diskPath )
  : cluster_{cluster}
  , diskID_{diskID}
  , diskPath_{diskPath}
  , files_{}
  , queue_{DISK_QUEUE_LENGTH}
  , writer_{}
  , threadStarted_{false}
  , start_{}
{
  // create bucket directory
  mkdir( cluster_.disk_bucket_directory( diskID_ ).c_str(), 0755 );

  // open all bucket output files for this disk
  for ( auto bkt : cluster_.myBuckets() ) {
    if ( cluster_.bucket_disk( bkt ) == diskID_ ) {
      size_t diskLocalID = diskLocalBucketID( bkt );
      if ( diskLocalID != files_.size() ) {
        throw runtime_error( "bucket to disk local mapping failed" );
      }
      // FIXME: Should we keep the buckets open for phase two?
      files_.emplace_back( cluster_.bucket_path( bkt ), O_WRONLY | O_CREAT | O_TRUNC,
                           S_IRUSR | S_IWUSR );
    }
  }
  print( "disk", (size_t) diskID_, diskPath, files_.size() );
}

DiskWriter::DiskWriter( DiskWriter && other )
  : cluster_{other.cluster_}
  , diskID_{other.diskID_}
  , diskPath_{other.diskPath_}
  , files_{move( other.files_ )}
  , queue_{move( other.queue_ )}
  , writer_{move( other.writer_ )}
  , threadStarted_{other.threadStarted_}
  , start_{other.start_}
{
  // FIXME: Hacky way to have some move semantics for vector storage
  if ( other.threadStarted_ ) {
    throw runtime_error( "Can't move a DiskWriter once thread started" );
  }
}

void DiskWriter::start( void )
{
  start_ = time_now();
  threadStarted_ = true;
  print( "p1", "disk-start", timestamp<ms>() );
  writer_ = thread( &DiskWriter::writeLoop, this );
}

DiskWriter::~DiskWriter( void )
{
  queue_.waitEmpty();
  queue_.close();
  if ( writer_.joinable() ) { writer_.join(); }
  print( "p1", "disk-end", timestamp<ms>(), time_diff<ms>( start_ ) );
}

void DiskWriter::waitDrained( bool fsync )
{
  queue_.waitEmpty();
  if ( fsync ) {
    for ( auto & f : files_ ) {
      f.fsync();
    }
  }
  print( "p1", "disk-drained", timestamp<ms>(), time_diff<ms>( start_ ) );
}

/* Convert a global bucket ID to a disk local bucket ID */
uint16_t DiskWriter::diskLocalBucketID( uint16_t bucket )
{
  return cluster_.bucket_local_id( bucket ) / cluster_.disks();
}

/* Queue a block to be written to disk */
void DiskWriter::send( block_t block )
{
  queue_.send( block );
}

/* Read from channel and write data to disk */
void DiskWriter::writeLoop( void )
{
  tdiff_t twrite = 0;
  try {
    while ( true ) {
      auto block = queue_.recv();
      uint16_t fileID = diskLocalBucketID( block.bucket );
      if ( fileID >= files_.size() ) {
        throw runtime_error( "fileID is not valid" );
      } else if ( block.len > 0 ) {
        auto t0 = time_now();
        files_[fileID].write_all( (char *) block.buf, block.len );
        files_[fileID].fsync();
        twrite += time_diff<ms>( t0 );
      }
      freeBlock( block.buf );
    }
  } catch ( const Channel<block_t>::closed_error & e ) {
    // EOF
  }
  print( "p1", "disk-write", timestamp<ms>(), twrite );
}
