#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "lib.hh"

int main(int argc, char* argv[]) {
  if (argc != 4 && argc != 5) {
    printf("Usage: ./write [file] [count] [block size] ([O_DIRECT?])\n");
    return EXIT_FAILURE;
  }
  int flags = argc == 5 ? O_DIRECT : 0;

  const char* file_name = argv[1];
  size_t num_block = atol(argv[2]);
  size_t block_size = atol(argv[3]);
  size_t file_size = num_block * block_size;

  int fd = open(file_name, O_CREAT | O_WRONLY | flags, 00644);
  if (fd < 0) {
    perror("Failed when opening file.");
    return EXIT_FAILURE;
  }

  char * data = (char *) aligned_alloc(ALIGN, block_size);
  for (size_t i = 0; i < block_size; ++i) {
    data[i] = i % 256;
  }

  struct timespec time_start, time_end;
  clock_gettime(CLOCK_MONOTONIC, &time_start);

  for (size_t i = 0; i < num_block; ++i) {
    ssize_t r = pwrite(fd, data, block_size, i * block_size);
    if (r == -1 || (size_t) r != block_size) {
      perror("Failed when writing.");
    }
  }
  if (fdatasync(fd) != 0) {
    perror("Failed when syncing.");
    return EXIT_FAILURE;
  }

  clock_gettime(CLOCK_MONOTONIC, &time_end);

  double duration = time_diff(time_start, time_end);
  printf("%.3f seconds\n%f MB/s\n", duration, file_size / duration / MB);

  if (close(fd) != 0) {
    perror("Failed when closing.");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
