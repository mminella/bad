#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <numeric>
#include <thread>

#include "tbb/task_group.h"

using namespace std;

using clk = chrono::high_resolution_clock;

constexpr size_t KEYSIZE = 10;
constexpr size_t KEYS_MB = 5000;
constexpr size_t ALIGN = 4096;
constexpr size_t MB = 1024 * 1024;

constexpr size_t DATASIZE = KEYS_MB * MB;

size_t time_diff( clk::time_point t )
{
  return chrono::duration_cast<chrono::milliseconds>(
    clk::now() - t ).count();
} 

void finish_test( string str, size_t lt, clk::time_point t )
{
  auto tt = time_diff( t );
  if ( tt > 0 ) {
    size_t mbs = DATASIZE * 1000 / tt / MB;
    cout << str << mbs << endl;
    cerr << lt << endl;
  }
}

void serial( char * key, char * rbuf )
{
  auto t0 = clk::now();
  size_t lt = 0;
  for ( size_t i = 0; i < DATASIZE; i+= KEYSIZE ) {
    lt += memcmp( key, &rbuf[i], KEYSIZE );
  }
  finish_test( "serial, pinned, 1, ", lt, t0 ); 
}

void parallel( char * key, char * rbuf, size_t splits )
{
  tbb::task_group tg;
  size_t * lt = new size_t[splits];

  auto t0 = clk::now();
  for ( size_t i = 0; i < splits; i++ ) {
    char * rbuf_i = rbuf + ( DATASIZE / splits ) * i;
    tg.run( [=]() {
      size_t r = 0, max = DATASIZE / splits;
      for ( size_t j = 0; j < max; j+= KEYSIZE ) {
        r += memcmp( key, &rbuf_i[j], KEYSIZE );
      }
      lt[i] = r;
      return r;
    });
  }
  tg.wait();
  size_t llt = accumulate( lt, lt + splits, 0 );
  finish_test( "parallel, pinned, " + to_string(splits) + ", ", llt, t0 );
}

int main( int argc, char ** argv )
{
  char * rbuf;
  posix_memalign( (void **) &rbuf, ALIGN, DATASIZE );
  for ( size_t i = KEYSIZE - 1; i < DATASIZE; i += KEYSIZE ) {
    rbuf[i] = 0;
  }
  
  serial( rbuf, rbuf );
  for ( size_t p = 2; p <= thread::hardware_concurrency(); p++ ) {
    parallel( rbuf, rbuf, p );
  }

  free( rbuf );
  return 0;
}


