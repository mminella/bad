#ifndef TIME_HH
#define TIME_HH

/*
 * Some nicer abstractions on top of C++11 chrono.
 */

#include <chrono>

using clk = std::chrono::high_resolution_clock;
using tdiff_t = uint64_t;

extern clk::time_point tstart;

// call at program start to capture relative start time.
void time_start( void );

static inline tdiff_t time_diff( clk::time_point t0 )
{
  auto t1 = clk::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>( t1 - t0 ).count();
}

static inline tdiff_t time_diff( clk::time_point t1, clk::time_point t0 )
{
  return std::chrono::duration_cast<std::chrono::milliseconds>( t1 - t0 ).count();
}

static inline tdiff_t time_snap( void )
{
  auto t1 = clk::now();
  return time_diff( t1, tstart );
}

#endif /* TIME_HH */
