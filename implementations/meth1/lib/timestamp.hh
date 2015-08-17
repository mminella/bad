#ifndef TIMESTAMP_HH
#define TIMESTAMP_HH

#include <ctime>
#include <cstdint>
#include <sys/time.h>

#include <chrono>

using clk = std::chrono::high_resolution_clock;
using ns  = std::chrono::nanoseconds;
using us  = std::chrono::microseconds;
using ms  = std::chrono::milliseconds;
using sec = std::chrono::seconds;
using tdiff_t = uint64_t;
using tpoint_t = clk::time_point;

extern const clk::time_point tstart;

/* Current time in milliseconds since the start of the program */
template <typename Duration>
inline tdiff_t timestamp( void )
{
  auto t1 = clk::now();
  return std::chrono::duration_cast<Duration>( t1 - tstart ).count();
}

/* Convert timespec to a time realtive to program start time */
template <typename Duration>
inline tdiff_t timestamp( const timespec & ts )
{
  auto dur = std::chrono::nanoseconds( ts.tv_sec * 1000000000 + ts.tv_nsec );
  auto clk_ts = std::chrono::time_point<clk>( dur );
  return std::chrono::duration_cast<Duration>( clk_ts - tstart ).count();
}

/* Convert timespec to a time realtive to program start time */
template <typename Duration>
inline tdiff_t timestamp( const timeval & tv )
{
  auto dur = std::chrono::microseconds( tv.tv_sec * 1000000 + tv.tv_usec );
  auto clk_ts = std::chrono::time_point<clk>( dur );
  return std::chrono::duration_cast<Duration>( clk_ts - tstart ).count();
}

/* Currrent time */
inline tpoint_t time_now( void )
{
  return clk::now();
}

/* Return the relative difference between the specified times */
template <typename Duration>
inline tdiff_t time_diff( clk::time_point t1, clk::time_point t0 )
{
  return std::chrono::duration_cast<Duration>( t1 - t0 ).count();
}

/* Return the time now relative to the specified time */
template <typename Duration>
inline tdiff_t time_diff( clk::time_point t0 )
{
  return time_diff<Duration>( clk::now(), t0 );
}

#endif /* TIMESTAMP_HH */
