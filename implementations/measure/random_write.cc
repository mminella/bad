#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "lib.h"

int main(int argc, char* argv[]) {
  int fd = open(argv[1], O_CREAT | O_WRONLY, 00777);
  if (fd < 0) {
    perror("Failed when opening file.");
    return -1;
  }

  unsigned char write_data[kBlockSize];
  for (size_t index = 0; index < kBlockSize; ++index) {
    write_data[index] = index % 256;
  }

  size_t shuffled_block_id[kTestBlock];
  FillRandomPermutation(shuffled_block_id, kTestBlock);

  struct timespec time_start, time_end;
  clock_gettime(CLOCK_MONOTONIC, &time_start);

  for (size_t index = 0; index < kTestBlock; ++index) {
    ssize_t result = pwrite(
        fd, write_data, kBlockSize, shuffled_block_id[index] * kBlockSize);
    if (result == -1 || (size_t) result != kBlockSize) {
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
         duration, kTestBlock / duration / 0x100000 * kBlockSize);

  if (close(fd) != 0) {
    perror("Failed when closing.");
    return -1;
  }
  return 0;
}
