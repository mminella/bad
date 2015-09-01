#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#include "tbb/task_group.h"

using namespace std;

using clk = chrono::high_resolution_clock;

constexpr size_t COPY_SIZE = 1024 * 1024 * 1000 * size_t(3);
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

void serial_pinned( char * rbuf, char * wbuf )
{
  auto t0 = clk::now();
  memcpy( wbuf, rbuf, COPY_SIZE );
  finish_test( "serial, pinned, 1, ", t0 ); 
}

void parallel_pinned( char * rbuf, char * wbuf, size_t splits )
{
  tbb::task_group tg;

  auto t0 = clk::now();
  for ( size_t i = 0; i < splits; i++ ) {
    char * rbuf_i = rbuf + (COPY_SIZE / splits) * i;
    char * wbuf_i = wbuf + (COPY_SIZE / splits) * i;
    tg.run( [=]() { memcpy( wbuf_i, rbuf_i, COPY_SIZE / splits ); } );
  }
  tg.wait();
  finish_test( "parallel, pinned, " + to_string(splits) + ", ", t0 );
}

void parallel_pinned_n( char * rbuf, size_t splits )
{
  char ** wbuf_i = new char*[splits];
  for ( size_t i = 0; i < splits; i++ ) {
    posix_memalign( (void **) &wbuf_i[i], ALIGN, COPY_SIZE / splits );
    memset( wbuf_i[i], 0, COPY_SIZE / splits );
  }

  tbb::task_group tg;

  auto t0 = clk::now();
  for ( size_t i = 0; i < splits; i++ ) {
    char * rbuf_i = rbuf + (COPY_SIZE / splits) * i;
    tg.run( [=]() { memcpy( wbuf_i[i], rbuf_i, COPY_SIZE / splits ); } );
  }
  tg.wait();
  finish_test( "parallel_n, pinned, " + to_string(splits) + ", ", t0 );

  for ( size_t i = 0; i < splits; i++ ) {
    free( wbuf_i[i] );
  }
  free( wbuf_i );
}

int main( int argc, char ** argv )
{
  char * rbuf;
  posix_memalign( (void **) &rbuf, ALIGN, COPY_SIZE );
  char * wbuf;
  posix_memalign( (void **) &wbuf, ALIGN, COPY_SIZE );
  memset( rbuf, 0, COPY_SIZE );
  memset( wbuf, 0, COPY_SIZE );

  serial_pinned( rbuf, wbuf );

  for ( size_t p = 2; p <= thread::hardware_concurrency(); p++ ) {
    parallel_pinned( rbuf, wbuf, p );
  }

  free( wbuf );

  for ( size_t p = 2; p <= thread::hardware_concurrency(); p++ ) {
    parallel_pinned_n( rbuf, p );
  }

  free( rbuf );
  return 0;
}

