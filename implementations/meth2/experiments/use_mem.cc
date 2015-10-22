
#include <cstring>
#include <cstdlib>

#include <string>
#include <iostream>

#include <unistd.h>
#include <sys/mman.h>

using namespace std;

int main(int argc, const char *argv[])
{
  uint64_t sz;

  if (argc != 2) {
    cout << "Usage: " << argv[0] << " GB_TO_USE" << endl;
    return 1;
  }

  sz = atoll(argv[1]);
  sz *= 1024;
  sz *= 1024;
  sz *= 1024;

  char *buf = (char *)mmap(NULL, sz,
			   PROT_READ|PROT_WRITE,
			   MAP_ANON|MAP_PRIVATE, -1, 0);
  if (buf == NULL) {
    cout << "Couldn't map memory!" << endl;
    return 1;
  }

  cout << "Touching memory" << endl;
  for (uint64_t i = 0; i < sz; i += 4096)
    buf[i] = 0x55;

  cout << "Ready!" << endl;
  while (1)
    sleep(100);
}

