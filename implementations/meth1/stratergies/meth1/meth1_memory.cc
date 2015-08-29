#include <cstdio>
#include <iostream>
#include <system_error>

#include "tune_knobs.hh"

#include "overlapped_io.hh"
#include "util.hh"

#include "record.hh"

#include "meth1_memory.hh"
#include "node.hh"

using namespace std;
using namespace meth1;

/* Get a count of how many disks this machine has */
size_t num_of_disks( void )
{
  FILE * fin = popen("ls /dev/xvd[b-z] | wc -l", "r");
  if ( not fin ) {
    throw runtime_error( "Couldn't count number of disks" );
  }
  char buf[256];
  fgets( buf, 256, fin );
  return max( (size_t) atoll( buf ), (size_t) 1 );
}

uint64_t calc_value_size( void )
{
  // Try to figure out how tightly we can allocate values (i.e., external
  // fragementation)
  vector <uint8_t *> vals;
  vector <uint64_t> diffs;
  for ( size_t i = 0; i < 6; i++ ) {
    vals.push_back( Rec::alloc_val() );
  }

  for ( auto v : vals ) {
    Rec::dealloc_val( v );
  }

  for ( size_t i = 1; i < vals.size(); i++ ) {
    diffs.push_back( uint64_t( vals[i] - vals[i-1] ) );
  }

  uint64_t val_len = *min_element( diffs.begin(), diffs.end() );
  cout << "record-value, " << val_len << endl;
  return min( val_len, 2 * Rec::VAL_LEN );
}

/* figure out max chunk size available */
uint64_t calc_record_space( void )
{
  // XXX: we should really do this at the backend node, not the client, but the
  // RPC interface isn't really setup that way.
  uint64_t memFree = memory_exists();
  uint64_t val_len = calc_value_size();

  // subtract reserved mem for OS & misc
  memFree -= Knobs::MEM_RESERVE;
  // subtract disk read buffers
  memFree -= ( OverlappedIO::BUFFER_SIZE * num_of_disks() );

  // divisor for r2 & r3 merge buffers
  uint64_t div1 = uint64_t( 2 ) * uint64_t( sizeof( Node::RR ) ) + val_len;
  // divisor for r1 sort buffer
  uint64_t div2 = ( uint64_t( sizeof( Node::RR ) ) + val_len )
    / Knobs::SORT_MERGE_RATIO;
  // divide by sort + merge buffers
  memFree /= ( div1 + div2 );

  return memFree;
}

