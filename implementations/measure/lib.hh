#ifndef LIB_HH
#define LIB_HH

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sys/syscall.h>
#include <linux/aio_abi.h>

#include <algorithm>
#include <sstream>
#include <string>

const size_t MB = 1024 * 1024;
const size_t ALIGN = 4096;

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

#endif /* LIB_HH */
