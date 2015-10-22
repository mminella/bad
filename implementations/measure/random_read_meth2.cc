#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "lib.hh"

using namespace std;

int fd;
size_t file_size;
size_t block_size = 512;
size_t num_block;
size_t *random_block;
size_t offset;

void
IOWorker()
{
  void * data;
  if (posix_memalign((void **)&data, /*PGSIZE*/4096, block_size) != 0) {
    exit(1);
  }
  while (true) {
    size_t idx;

    idx = __sync_fetch_and_add(&offset, 1);
    if (idx >= num_block) {
      break;
    }

    ssize_t r = pread(fd, data, block_size, random_block[idx] * block_size);
    if ( r == -1 ) {
      perror("pread:");
      exit(EXIT_FAILURE);
    }
  }

  free(data);
}

void
Benchmark(int oio)
{
  vector<thread> threads;

  // Start
  offset = 0;
  for (int i = 0; i < oio; i++) {
    threads.emplace_back(&IOWorker);
  }

  struct timespec time_start, time_end;
  get_timespec(&time_start);

  // Wait for all threads to complete
  for (auto &t : threads) {
    t.join();
  }

  get_timespec(&time_end);

  double dur = time_diff(time_start, time_end);
  double iops = num_block / dur;
  // read?, sync?, random?, block, depth, odirect, MB/s
  printf("block %ld, oio %d, iops %f\n",
	  block_size, oio, iops);
}

int main(int argc, char* argv[]) {
  if (argc != 2 && argc != 3) {
    printf("Usage: ./read [file] ([O_DIRECT?])\n");
    return EXIT_FAILURE;
  }
  bool odirect = argc == 3;
  int flags = odirect ? O_DIRECT | O_RDONLY : O_RDONLY;

  const char* file_name = argv[1];
  fd = open(file_name, flags);
  if (fd < 0) {
    perror("Failed when opening file");
    return EXIT_FAILURE;
  }

  struct stat stat_buffer;
  if (fstat(fd, &stat_buffer) != 0) {
    perror("Failed when fstat.");
    return EXIT_FAILURE;
  }
  file_size = stat_buffer.st_size;
  num_block = file_size / block_size;
  random_block = new size_t[num_block];

  FillRandomPermutation(random_block, num_block);

  //for (int oio = 1; oio <= 64; oio *= 2) {
    Benchmark(16);
  //}

  if (close(fd) != 0) {
    perror("Failed when closing.");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

