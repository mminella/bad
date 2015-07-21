#ifndef LIB_H_
#define LIB_H_

#include <stdlib.h>
#include <time.h>

#include <sys/syscall.h>
#include <linux/aio_abi.h>

#include <algorithm>
#include <sstream>
#include <string>

/*
 * Default parameter:
 * file size = 10G = 10737418240 Byte.
 * block size = 1M = 1048576
 * fly requests = 64
 */

inline double GetTimeDuration(
    const struct timespec& start, const struct timespec& end) {
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

inline size_t TranslateToInt(const char* input) {
  std::istringstream conv(input);
  size_t result;
  conv >> result;
  return result;
}



#endif  // LIB_H_
