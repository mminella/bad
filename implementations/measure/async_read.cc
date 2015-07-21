#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "lib.h"

int main(int argc, char* argv[]) {
  if (argc != 4) {
    printf("Usage: ./async_read [file name] [block size] [fly requests]\n");
    return -1;
  }
  const char* file_name = argv[1];
  int fd = open(file_name, O_CREAT | O_RDONLY | O_DIRECT, 00777);
  struct stat stat_buffer;
  if (fstat(fd, &stat_buffer) != 0) {
    perror("Failed when fstat.");
    return -1;
  }
  size_t file_size = stat_buffer.st_size;
  size_t block_size = TranslateToInt(argv[2]);
  int fly_requests = TranslateToInt(argv[3]);
  if (file_size % block_size != 0) {
    printf("File size must be a multiple of block size.\n");
    return -1;
  }
  size_t num_block = file_size / block_size;

  aio_context_t ctx = 0;
  if (io_setup(fly_requests, &ctx) < 0) {
    perror("Failed when io_setup.");
    return -1;
  }
  struct iocb* cb = new struct iocb[fly_requests];
  struct iocb** cbs = new struct iocb*[fly_requests];
  struct io_event* events = new struct io_event[fly_requests];
  for (int i = 0; i < fly_requests; i++) {
    memset(&cb[i], 0, sizeof(cb[i]));
    cb[i].aio_fildes = fd;
    cb[i].aio_lio_opcode = IOCB_CMD_PREAD;
    cb[i].aio_nbytes = block_size;
    void* buffer;
    if (posix_memalign(&buffer, block_size, block_size) < 0) {
      perror("Failed when memalighn.");
    }
    cb[i].aio_buf = (uint64_t) buffer;
  }

  struct timespec time_start, time_end;
  clock_gettime(CLOCK_MONOTONIC, &time_start);
  int submitted_request = 0;
  int finished_request = 0;
  for (int j = 0; j < fly_requests && j < num_block; ++j) {
    cbs[j] = &cb[j];
    cbs[j]->aio_offset = (submitted_request++) * block_size;
  }
  if (io_submit(ctx, submitted_request, cbs) != fly_requests) {
    perror("Failed in io_submit.");
    return -1;
  }
  while (finished_request < num_block) {
    int return_events = io_getevents(ctx, 0, fly_requests, events, NULL);
    if (return_events < 0) {
      perror("Failed in io_getevents.");
      return -1;
    }
    finished_request += return_events;
    int new_events = submitted_request + return_events > num_block
        ? num_block - submitted_request : return_events;
    for (int j = 0; j < new_events; ++j) {
      cbs[j]->aio_offset = (submitted_request++) * block_size;
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
         duration, file_size / duration / 0x100000);

  if (close(fd) != 0) {
    perror("Failed when closing.");
    return -1;
  }
  return 0;
}
