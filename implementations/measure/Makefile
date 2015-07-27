CXX=c++
CXXFLAGS=-O3 -lrt
BINARIES= \
	read \
	random_read \
	write \
	random_write \
	async_random_read \
	async_read

all: $(BINARIES)

.PHONY: clean
clean:
	rm -f *.o
	rm -f $(BINARIES)
