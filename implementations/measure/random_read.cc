#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "lib.h"

int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Usage: ./random_read [file name] [block size]\n");
    return -1;
  }
  const char* file_name = argv[1];
  int fd = open(file_name, O_CREAT | O_RDONLY, 00777);
  struct stat stat_buffer;
  if (fstat(fd, &stat_buffer) != 0) {
    perror("Failed when fstat.");
    return -1;
  }
  size_t file_size = stat_buffer.st_size;
  size_t block_size = TranslateToInt(argv[2]);
  if (file_size % block_size != 0) {
    printf("File size must be a multiple of block size.\n");
    return -1;
  }
  size_t num_block = file_size / block_size;

  if (fd < 0) {
    perror("Failed when opening file.");
    return -1;
  }

  unsigned char* read_data = new unsigned char[block_size];

  size_t* shuffled_block_id = new size_t[num_block];
  FillRandomPermutation(shuffled_block_id, num_block);

  struct timespec time_start, time_end;
  clock_gettime(CLOCK_MONOTONIC, &time_start);

  for (size_t index = 0; index < num_block; ++index) {
    ssize_t result = pread(
        fd, read_data, block_size, shuffled_block_id[index] * block_size);
    if (result == -1 || (size_t) result != block_size) {
      perror("Failed when reading.");
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
