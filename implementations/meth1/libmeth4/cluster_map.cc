#include <cmath>
#include <cstring>
#include <vector>

#include "file.hh"
#include "util.hh"

#include "record.hh"

#include "cluster_map.hh"
#include "config_file.hh"
#include "meth4_knobs.hh"

using namespace std;

size_t num_of_disks( void )
{
  FILE * fin = popen("ls /dev/xvd[b-z] 2>/dev/null | wc -l", "r");
  if ( not fin ) {
    throw std::runtime_error( "Couldn't count number of disks" );
  }
  char buf[256];
  char * n = fgets( buf, 256, fin );
  if ( n == nullptr ) {
    throw std::runtime_error( "Couldn't count number of disks" );
  }
  return std::max( (size_t) atoll( buf ), (size_t) 1 );
}

vector<File> openFiles( vector<string> files )
{
  vector<File> data;
  for ( const auto & f : files ) {
    data.emplace_back( f, O_RDONLY, File::DIRECT );
  }
  return data;
}

vector<string> extractDiskPaths( vector<string> files )
{
  vector<string> diskPaths;
  for ( auto & f : files ) {
    size_t i = f.find_last_of( '/' );
    if ( i == string::npos ) {
      throw runtime_error( "Invalid path to record file" );
    }
    diskPaths.emplace_back( f.substr( 0, i ) );
  }
  return diskPaths;
}

size_t recordsInFiles( vector<File> & files )
{
  size_t nrecs = 0;
  for ( const auto & f : files ) {
    nrecs += f.size() / Rec::SIZE;
  }
  return nrecs;
}

size_t calcMaxSortSize( size_t disks )
{
  // By 2 since we want to overlap reading in one bucket while sorting the
  // previous one.
  // FIXME: Two should be configurable, but requires changing sort.hh as well.
  size_t mem = memory_exists() - Knobs4::MEM_RESERVE;
  return ( mem / Rec::SIZE ) / disks / 2;
}

size_t bucketsPerNode( size_t recordsPerNode, size_t disksPerNode )
{
  size_t recordsSortMax = calcMaxSortSize( disksPerNode );
  size_t recordsPerDisk =
    ceil( double( recordsPerNode ) / double( disksPerNode ) );
  size_t bucketsPerDisk =
    ceil( double( recordsPerDisk ) / double( recordsSortMax ) );

  size_t minBckts = ClusterMap::MIN_BKTS_PER_DISK;
  return disksPerNode * max( minBckts, bucketsPerDisk );
}

ClusterMap::ClusterMap( size_t myID, string configFile,
                        vector<string> dataFiles )
  : myID_{myID}
  , backends_{ConfigFile::parse( configFile )}
  , recFiles_{openFiles( dataFiles )}
  , diskPaths_{extractDiskPaths( dataFiles )}
  , disks_{min( num_of_disks(), dataFiles.size() )}
  , recordsLocally_{recordsInFiles( recFiles_ )}
  , bucketsPerNode_{bucketsPerNode( recordsLocally_, disks_ )}
  , shards_{calculateShards( bucketsPerNode_ * backends_.size() )}
  , preShards_{precomputeFirstByte( shards_ )}
  , myBuckets_{calcMyBuckets( myID, backends_.size(), disks_,
                             bucketsPerNode_ * backends_.size() )}
{
  if ( ( bucketsPerNode_ * nodes() ) > UINT16_MAX ) {
    throw runtime_error( "Can't have that many buckets" );
  } else if ( nodes() > UINT16_MAX ) {
    throw runtime_error( "Can't have that many nodes" );
  } else if ( bucketsPerNode_ % disks_ != 0 ) {
    throw runtime_error( "Number of buckets doesn't map to number of disks" );
  }
}

uint16_t ClusterMap::myID( void ) const noexcept
{
  return myID_;
}

const vector<uint16_t> & ClusterMap::myBuckets( void ) const noexcept
{
  return myBuckets_;
}

size_t ClusterMap::bucket_local_id( uint16_t bkt ) const noexcept
{
  return bkt / nodes();
}

uint8_t ClusterMap::bucket_disk( uint16_t bkt ) const noexcept
{
  return ( bkt / nodes() ) % disks_;
}

uint16_t ClusterMap::bucket_node( uint16_t bkt ) const noexcept
{
  return bkt % nodes();
}

vector<File> & ClusterMap::files( void ) noexcept
{
  return recFiles_;
}

const vector<std::string> & ClusterMap::disk_paths( void ) const noexcept
{
  return diskPaths_;
}

const string BUCKET_DIR = "buckets";
const string BUCKET_EXT = ".bucket";
const string BUCKET_SORTED = "sorted.";

string ClusterMap::disk_bucket_directory( size_t diskID ) const noexcept
{
  return diskPaths_[diskID] + "/" + BUCKET_DIR;
}

string ClusterMap::bucket_path( uint16_t bkt ) const noexcept
{
  size_t diskID = bucket_disk( bkt );
  return disk_bucket_directory( diskID ) + "/" + to_string(bkt) + BUCKET_EXT;
}

string ClusterMap::sorted_bucket_path( uint16_t bkt ) const noexcept
{
  size_t diskID = bucket_disk( bkt );
  return disk_bucket_directory( diskID ) + "/" + BUCKET_SORTED
    + to_string(bkt) + BUCKET_EXT;
}

size_t ClusterMap::disks( void ) const noexcept
{
  return disks_;
}

vector<Address> ClusterMap::addresses( void ) const noexcept
{
  return backends_;
}

size_t ClusterMap::nodes( void ) const noexcept
{
  return backends_.size();
}

size_t ClusterMap::buckets( void ) const noexcept
{
  return shards_.size();
}

using uint128_t = __uint128_t;

uint128_t maxKey( void ) noexcept
{
  uint128_t max = 1;
  return (max << ( Rec::KEY_LEN * 8) ) - 1;
}

void reverse_bytes( void * bytes, size_t len )
{
  uint8_t * s = (uint8_t *) bytes;
  uint8_t * e = s + len - 1;
  while ( s < e ) {
    uint8_t tmp = *s;
    *s++ = *e;
    *e-- = tmp;
  }
}

ClusterMap::shards_t ClusterMap::calculateShards( size_t buckets )
  const noexcept
{
  // Make use of clang / gcc emulated 128 bit types. Doesn't generalize, but
  // makes it very easy to calculate the map and has no dependencies.
  static_assert( sizeof( uint128_t ) >= Rec::KEY_LEN,
    "record key size too large" );

  shards_t shards;
  uint128_t max = maxKey();
  uint128_t split = max / buckets;
  uint128_t shard_key = 0;

  for ( size_t i = 0; i < buckets - 1; i++ ) {
    shard_key += split;

    shard_t s( i );
    memcpy( s.k_, (uint8_t *) &shard_key, 10 );
    reverse_bytes( s.k_, 10 );
    shards.push_back( s );
  }

  // do final one explicitly to ensure no rouding errors (key == max)
  shard_t s( buckets - 1 );
  memcpy( s.k_, (uint8_t *) &max, 10 );
  shards.push_back( s );

  return shards;
}
ClusterMap::pre_shards_t ClusterMap::precomputeFirstByte( shards_t shards )
  const noexcept
{
  pre_shards_t preShards( 256 );
  uint16_t lastb0 = 0;

  for ( auto & s : shards ) {
    uint16_t b0 = s.k_[0];
    for ( uint16_t bi = lastb0; bi <= b0; bi++ ) {
      preShards[bi].push_back( s );
    }
    lastb0 = b0;
  }

  return preShards;
}

vector<uint16_t> ClusterMap::calcMyBuckets( uint16_t id, size_t nodes,
                                            size_t disks, size_t buckets )
                                            const noexcept
{
  if ( buckets % nodes != 0 or buckets % ( nodes * disks ) != 0 ) {
    throw runtime_error( "Number of buckets doesn't map cleanly to cluster" );
  }

  vector<uint16_t> myBuckets;
  for ( size_t i = 0; i < buckets; i += nodes ) {
    myBuckets.push_back( i + id );
  }
  return myBuckets;
}

uint16_t ClusterMap::bucket( const uint8_t * key ) const noexcept
{
  size_t b;
  // shards_t & shards = shards_;
  const shards_t & shards = preShards_[key[0]];
  for ( b = 0; b < shards.size() - 1; b++ ) {
    if ( memcmp( key, shards[b].k_, Rec::KEY_LEN ) <= 0 ) {
      break;
    }
  }
  return shards[b].id_;
}
