#include <cstdio>
#include <iostream>
#include <system_error>

#include "tune_knobs.hh"

#include "overlapped_io.hh"
#include "util.hh"

#include "meth1_memory.hh"
#include "node.hh"

using namespace std;
using namespace meth1;

/* Get a count of how many disks this machine has */
size_t num_of_disks( void )
{
  FILE * fin = popen("ls /dev/xvd* | wc -l", "r");
  if ( not fin ) {
    throw runtime_error( "Couldn't count number of disks" );
  }
  char buf[256];
  fgets( buf, 256, fin );
  return max( (size_t) atoll( buf ), (size_t) 1 );
}

/* figure out max chunk size available */
uint64_t calc_record_space( void )
{
  // XXX: we should really do this at the backend node, not the client, but the
  // RPC interface isn't really setup that way.
  uint64_t memFree = memory_free();

  // subtract reserved mem for OS & misc
  memFree -= Knobs::MEM_RESERVE;
  // subtract disk read buffers
  memFree -= OverlappedIO::BUFFER_SIZE * num_of_disks();
  // divide by record size
  memFree /= ( sizeof(Node::RR) + Rec::VAL_LEN );
  // subtract the extra sort buffer
  memFree = ( memFree * Knobs::SORT_MERGE_RATIO )
    / ( Knobs::SORT_MERGE_RATIO + 1 );
  
  return memFree;
}

