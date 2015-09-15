#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#include "tbb/task_group.h"

using namespace std;

using clk = chrono::high_resolution_clock;

constexpr size_t COPY_SIZE = 1024 * 1024 * 1000 * size_t(5);
constexpr size_t ALIGN = 4096;
constexpr size_t MB = 1024 * 1024;

size_t time_diff( clk::time_point t )
{
  return chrono::duration_cast<chrono::milliseconds>(
    clk::now() - t ).count();
} 

void finish_test( string str, clk::time_point t )
{
  auto tt = time_diff( t );
  if ( tt > 0 ) {
    size_t mbs = COPY_SIZE * 1000 / tt / MB;
    cout << str << mbs << endl;
  }
}

void parallel_unpinned( char * rbuf, char * wbuf, size_t splits )
{
  tbb::task_group tg;

  auto t0 = clk::now();
  for ( size_t i = 0; i < splits; i++ ) {
    char * rbuf_i = rbuf + (COPY_SIZE / splits) * i;
    char * wbuf_i = wbuf + (COPY_SIZE / splits) * i;
    tg.run( [=]() { memcpy( wbuf_i, rbuf_i, COPY_SIZE / splits ); } );
  }
  tg.wait();
  finish_test( "parallel, unpinned, " + to_string(splits) + ", ", t0 );
}

int main( int argc, char ** argv )
{
  if ( argc != 2 ) {
    cout << "Usage: " << argv[0] << " [threads]" << endl;
    return 1;
  }
  
  size_t splits = atoll( argv[1] );

  char * rbuf;
  char * wbuf;
  posix_memalign((void **)&rbuf, ALIGN, COPY_SIZE); 
  posix_memalign((void **)&wbuf, ALIGN, COPY_SIZE); 
  memset( rbuf, 0, COPY_SIZE );

  parallel_unpinned( rbuf, wbuf, splits );

  free( rbuf );
  return 0;
}

