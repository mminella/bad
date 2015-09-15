#ifndef LIB_HH
#define LIB_HH

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#if defined(__linux__)
#include <sys/syscall.h>
#include <linux/aio_abi.h>
#endif

#if defined(__MACH__)
#include <mach/mach.h>
#include <mach/clock.h>
#endif

#include <algorithm>
#include <sstream>
#include <string>

const size_t MB = 1024 * 1024;
const size_t ALIGN = 4096;

#ifndef O_DIRECT
#define O_DIRECT 0
#endif

#ifndef __linux__
#define fdatasync fsync
#endif

void
get_timespec(struct timespec *ts)
{
#ifdef __MACH__
    clock_serv_t clk;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &clk);
    clock_get_time(clk, &mts);
    mach_port_deallocate(mach_task_self(), clk);
    ts->tv_sec = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;
#else
    clock_gettime(CLOCK_MONOTONIC , ts);
#endif
}

inline double time_diff( const struct timespec& start,
                         const struct timespec& end ) {
  double duration = end.tv_nsec;
  duration -= start.tv_nsec;
  duration /= 1e9;
  duration += end.tv_sec - start.tv_sec;
  return duration;
}

inline void FillRandomPermutation(size_t index_array[], size_t max_index) {
  for (size_t index = 0; index < max_index; ++index) {
    index_array[index] = index;
  }
  for (size_t index = 0; index < max_index; ++index) {
    int random_index = ((rand() % max_index) + max_index) % max_index;
    std::swap(index_array[index], index_array[random_index]);
  }
}

#if defined(__linux__)

inline int io_setup(unsigned nr, aio_context_t *ctxp) {
  return syscall(__NR_io_setup, nr, ctxp);
}

inline int io_destroy(aio_context_t ctx) {
  return syscall(__NR_io_destroy, ctx);
}

inline int io_submit(aio_context_t ctx, long nr, struct iocb **iocbpp) {
  return syscall(__NR_io_submit, ctx, nr, iocbpp);
}

inline int io_getevents(aio_context_t ctx, long min_nr, long max_nr,
                        struct io_event *events, struct timespec *timeout) {
  return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
}

#endif

#endif /* LIB_HH */
