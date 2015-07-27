#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "lib.hh"

int main(int argc, char* argv[]) {
  if (argc != 3 && argc != 4) {
    printf("Usage: ./read [file] [block size] ([O_DIRECT?])\n");
    return EXIT_FAILURE;
  }
  int flags = argc == 4 ? O_DIRECT | O_RDONLY : O_RDONLY;

  const char* file_name = argv[1];
  int fd = open(file_name, flags);
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
  size_t num_block = file_size / block_size;

  size_t* random_block = new size_t[num_block];
  FillRandomPermutation(random_block, num_block);

  struct timespec time_start, time_end;
  clock_gettime(CLOCK_MONOTONIC, &time_start);

  void * data = aligned_alloc(ALIGN, block_size);
  for (size_t i = 0; i < num_block; ++i) {
    ssize_t r = pread(fd, data, block_size, random_block[i] * block_size);
    if (r == -1) {
      perror("Failed when reading");
    } else if ( r == 0 ) {
      break; // EOF
    }
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
