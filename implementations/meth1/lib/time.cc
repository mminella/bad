#include <chrono>

#include "time.hh"

using namespace std;

clk::time_point tstart;

void time_start( void )
{
  tstart = clk::now();
}

