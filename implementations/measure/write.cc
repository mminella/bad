#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "lib.h"

int main(int argc, char* argv[]) {
  if (argc != 4) {
    printf("Usage: ./write [file name] [file size] [block size]\n");
    return -1;
  }
  const char* file_name = argv[1];
  size_t file_size = TranslateToInt(argv[2]);
  size_t block_size = TranslateToInt(argv[3]);
  if (file_size % block_size != 0) {
    printf("File size must be a multiple of block size.\n");
    return -1;
  }
  size_t num_block = file_size / block_size;
  int fd = open(file_name, O_CREAT | O_WRONLY, 00777);
  if (fd < 0) {
    perror("Failed when opening file.");
    return -1;
  }

  unsigned char* write_data = new unsigned char[block_size];
  for (size_t index = 0; index < block_size; ++index) {
    write_data[index] = index % 256;
  }

  struct timespec time_start, time_end;
  clock_gettime(CLOCK_MONOTONIC, &time_start);

  for (size_t index = 0; index < num_block; ++index) {
    ssize_t result = pwrite(fd, write_data, block_size, index * block_size);
    if (result == -1 || (size_t) result != block_size) {
      perror("Failed when writing.");
    }
  }
  if (fdatasync(fd) != 0) {
    perror("Failed when syncing.");
    return -1;
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
