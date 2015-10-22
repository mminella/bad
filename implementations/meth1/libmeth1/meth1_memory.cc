#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <system_error>

#include "tune_knobs.hh"

#include "circular_io.hh"
#include "sync_print.hh"
#include "util.hh"

#include "record.hh"

#include "meth1_memory.hh"
#include "node.hh"

using namespace std;
using namespace meth1;

/* Get a count of how many disks this machine has */
size_t num_of_disks( void )
{
  FILE * fin = popen("ls /dev/xvd[b-z] 2>/dev/null | wc -l", "r");
  if ( not fin ) {
    throw runtime_error( "Couldn't count number of disks" );
  }
  char buf[256];
  char * n = fgets( buf, 256, fin );
  if ( n == nullptr ) {
    throw std::runtime_error( "Couldn't count number of disks" );
  }
  return max( (size_t) atoll( buf ), (size_t) 1 );
}

/* Try to figure out how tightly we can allocate values */
size_t calc_value_size( void )
{
  // vector <uint8_t *> vals;
  // vector <size_t> diffs;
  // for ( size_t i = 0; i < 6; i++ ) {
  //   vals.push_back( Rec::alloc_val() );
  // }
  //
  // for ( auto v : vals ) {
  //   Rec::dealloc_val( v );
  // }
  //
  // for ( size_t i = 1; i < vals.size(); i++ ) {
  //   diffs.push_back( abs( vals[i] - vals[i-1] ) );
  // }
  //
  // size_t val_len = *min_element( diffs.begin(), diffs.end() );
  // print( "record-value", val_len );
  //
  // return min( val_len, 2 * Rec::VAL_LEN );

  // XXX: Just hard-code for now due to difference between client & backend
  size_t val_len = 112;
  print( "record-value", val_len );
  return val_len;
}

/* figure out max chunk size available */
uint64_t calc_record_space( void )
{
  static_assert( sizeof( uint64_t ) >= sizeof( size_t ), "uint64_t >= size_t" );

  uint64_t memExists = memory_exists();
  uint64_t memFree = memExists;
  uint64_t val_len = calc_value_size();

  // remove OS space
  if ( Knobs::MEM_RESERVE > memFree ) {
    throw runtime_error( "Not enough memory" );
  }
  memFree -= Knobs::MEM_RESERVE;

  // remove net buffers
  uint64_t bufSizes = ( Knobs::IO_BUFFER_NETW * Rec::SIZE * 2 );
  if ( bufSizes > memFree ) {
    throw runtime_error( "Not enough memory" );
  }
  memFree -= bufSizes;

  // remove disk buffers
  bufSizes = ( CircularIO::BLOCK * Knobs::DISK_BLOCKS * num_of_disks() );
  if ( bufSizes > memFree ) {
    throw runtime_error( "Not enough memory" );
  }
  memFree -= bufSizes;

  // divisor for r2 & r3 merge buffers
  uint64_t div1 = uint64_t( 2 ) * uint64_t( sizeof( Node::RR ) ) + val_len;
  // divisor for r1 sort buffer
  uint64_t div2 = ( uint64_t( sizeof( Node::RR ) ) + val_len )
    / Knobs::SORT_MERGE_RATIO;

  // divide by sort + merge buffers
  memFree /= ( div1 + div2 );

  if ( memFree > memExists ) {
    throw runtime_error( "Bad memory calculation" );
  }
  return memFree;
}

