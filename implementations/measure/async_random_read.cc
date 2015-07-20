#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "lib.h"

int main(int argc, char* argv[]) {
  int fd = open(argv[1], O_CREAT | O_RDONLY | O_DIRECT, 00777);
  if (fd < 0) {
    perror("Failed when opening file.");
    return -1;
  }

  size_t shuffled_block_id[kTestBlock];
  FillRandomPermutation(shuffled_block_id, kTestBlock);

  const int kFlyRequests = 64;
  aio_context_t ctx = 0;
  if (io_setup(kFlyRequests, &ctx) < 0) {
    perror("Failed when io_setup.");
    return -1;
  }
  struct iocb cb[kFlyRequests];
  struct iocb* cbs[kFlyRequests];
  struct io_event events[kFlyRequests];
  for (int i = 0; i < kFlyRequests; i++) {
    memset(&cb[i], 0, sizeof(cb[i]));
    cb[i].aio_fildes = fd;
    cb[i].aio_lio_opcode = IOCB_CMD_PREAD;
    cb[i].aio_nbytes = kBlockSize;
    void* buffer;
    if (posix_memalign(&buffer, kBlockSize, kBlockSize) < 0) {
      perror("Failed when memalighn.");
    }
    cb[i].aio_buf = (uint64_t) buffer;
  }

  struct timespec time_start, time_end;
  clock_gettime(CLOCK_MONOTONIC, &time_start);
  int submitted_request = 0;
  int finished_request = 0;
  for (int j = 0; j < kFlyRequests && j < kTestBlock; ++j) {
    cbs[j] = &cb[j];
    cbs[j]->aio_offset = shuffled_block_id[submitted_request++] * kBlockSize;
  }
  if (io_submit(ctx, submitted_request, cbs) != kFlyRequests) {
    perror("Failed in io_submit.");
    return -1;
  }
  while (finished_request < kTestBlock) {
    int return_events = io_getevents(ctx, 0, kFlyRequests, events, NULL);
    if (return_events < 0) {
      perror("Failed in io_getevents.");
      return -1;
    }
    finished_request += return_events;
    int new_events = submitted_request + return_events > kTestBlock
        ? kTestBlock - submitted_request : return_events;
    for (int j = 0; j < new_events; ++j) {
      cbs[j]->aio_offset = shuffled_block_id[submitted_request++] * kBlockSize;
    }
    if (new_events > 0) {
      if (io_submit(ctx, new_events, cbs) != new_events) {
        perror("Failed in io_submit.");
        return -1;
      }
    }
  }

  clock_gettime(CLOCK_MONOTONIC, &time_end);

  double duration = GetTimeDuration(time_start, time_end);
  printf("%.3f second\n%f MB/s\n",
         duration, kTestBlock / duration / 0x100000 * kBlockSize);

  if (close(fd) != 0) {
    perror("Failed when closing.");
    return -1;
  }
  return 0;
}
