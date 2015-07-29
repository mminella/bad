#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "lib.hh"

int main(int argc, char* argv[]) {
  if (argc != 4 && argc != 5) {
    printf("Usage: ./async_read [file] [block size] [queue depth] " \
      "([O_DIRECT])\n");
    return EXIT_FAILURE;
  }
  int flags = argc == 5 ? O_DIRECT | O_RDONLY : O_RDONLY;

  const char* file_name = argv[1];
  int fd = open(file_name, O_RDONLY);
  if (fd < 0) {
    perror("Failed when opening file");
    return EXIT_FAILURE;
  }

  struct stat stat_buffer;
  if (fstat(fd, &stat_buffer) != 0) {
    perror("Failed when fstat.");
    return EXIT_FAILURE;
  }
  size_t file_size = stat_buffer.st_size;
  size_t block_size = atol(argv[2]);
  int qdepth = atoi(argv[3]);
  size_t num_block = file_size / block_size;
 
  size_t* random_block = new size_t[num_block];
  FillRandomPermutation(random_block, num_block);

  aio_context_t ctx = 0;
  if (io_setup(qdepth, &ctx) < 0) {
    perror("Failed when io_setup.");
    return EXIT_FAILURE;
  }

  struct iocb* cb = new struct iocb[qdepth];
  struct iocb** cbs = new struct iocb*[qdepth];
  struct io_event* events = new struct io_event[qdepth];
  for (int i = 0; i < qdepth; i++) {
    memset(&cb[i], 0, sizeof(cb[i]));
    cb[i].aio_fildes = fd;
    cb[i].aio_lio_opcode = IOCB_CMD_PREAD;
    cb[i].aio_nbytes = block_size;
    void* buffer = aligned_alloc(ALIGN, block_size);
    cb[i].aio_buf = (uint64_t) buffer;
  }

  struct timespec time_start, time_end;
  clock_gettime(CLOCK_MONOTONIC, &time_start);

  int submitted_request = 0;
  int finished_request = 0;
  for (int j = 0; j < qdepth && j < num_block; ++j) {
    cbs[j] = &cb[j];
    cbs[j]->aio_offset = random_block[submitted_request++] * block_size;
  }

  if (io_submit(ctx, submitted_request, cbs) != qdepth) {
    perror("Failed in io_submit.");
    return EXIT_FAILURE;
  }

  while (finished_request < num_block) {
    int return_events = io_getevents(ctx, 0, qdepth, events, NULL);
    if (return_events < 0) {
      perror("Failed in io_getevents.");
      return EXIT_FAILURE;
    }
    finished_request += return_events;
    int new_events = submitted_request + return_events > num_block
        ? num_block - submitted_request : return_events;
    for (int j = 0; j < new_events; ++j) {
      cbs[j]->aio_offset = random_block[submitted_request++] * block_size;
    }
    if (new_events > 0) {
      if (io_submit(ctx, new_events, cbs) != new_events) {
        perror("Failed in io_submit.");
        return EXIT_FAILURE;
      }
    }
  }

  clock_gettime(CLOCK_MONOTONIC, &time_end);

  double dur = time_diff(time_start, time_end);
  double mbs = file_size / dur / MB;
  // read?, sync?, random?, block, depth, MB/s
  printf("%c, %c, %c, %ld, %d, %f\n", 'r', 'a', 'r', block_size, qdepth, mbs);

  if (close(fd) != 0) {
    perror("Failed when closing.");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
